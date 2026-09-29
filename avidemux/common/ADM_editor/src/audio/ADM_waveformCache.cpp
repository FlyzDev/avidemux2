#include "ADM_waveformCache.h"

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>

// Avidemux core provides UTF-8-aware file operations on Windows. Keep the
// cache module independent from generated core headers so its format tests can
// still be built as a small standalone binary.
extern "C"
{
extern FILE *ADM_fopen(const char *file, const char *mode);
extern size_t ADM_fread(void *ptr, size_t size, size_t n, FILE *stream);
extern size_t ADM_fwrite(const void *ptr, size_t size, size_t n, FILE *stream);
extern int ADM_fclose(FILE *file);
extern uint8_t ADM_eraseFile(const char *name);
}
extern uint8_t ADM_renameFile(const char *source, const char *target);

namespace
{
static const char kMagic[8] = {'A','D','M','W','F','0','0','1'};
static const uint32_t kFormatVersion = 1;
static const uint32_t kMaxTracks = 64;
static const uint32_t kMaxChannelsPerTrack = 64;
static const uint32_t kMaxBinsPerChannel = 1024 * 1024;
static const uint32_t kMaxKeyBytes = 256;

class Fnv64
{
public:
    Fnv64() : value(1469598103934665603ULL) {}

    void bytes(const void *ptr, size_t size)
    {
        const unsigned char *p = static_cast<const unsigned char *>(ptr);
        for (size_t i = 0; i < size; ++i)
        {
            value ^= static_cast<uint64_t>(p[i]);
            value *= 1099511628211ULL;
        }
    }

    template <typename T>
    void pod(const T &v)
    {
        bytes(&v, sizeof(v));
    }

    void string(const std::string &s)
    {
        const uint64_t size = static_cast<uint64_t>(s.size());
        pod(size);
        if (!s.empty())
            bytes(s.data(), s.size());
    }

    uint64_t value;
};

template <typename T>
bool writePod(FILE *out, const T &value)
{
    return out && ADM_fwrite(&value, sizeof(value), 1, out) == 1;
}

template <typename T>
bool readPod(FILE *in, T *value)
{
    return in && value && ADM_fread(value, sizeof(*value), 1, in) == 1;
}

bool writeString(FILE *out, const std::string &value)
{
    if (!out || value.size() > kMaxKeyBytes)
        return false;
    const uint32_t size = static_cast<uint32_t>(value.size());
    if (!writePod(out, size))
        return false;
    return !size || ADM_fwrite(value.data(), 1, size, out) == size;
}

bool readString(FILE *in, std::string *value)
{
    if (!in || !value)
        return false;
    uint32_t size = 0;
    if (!readPod(in, &size) || size > kMaxKeyBytes)
        return false;
    value->assign(size, '\0');
    return !size || ADM_fread(&(*value)[0], 1, size, in) == size;
}
}

std::string ADM_waveformCacheKey(const ADM_WaveformSnapshot &snapshot,
                                 uint32_t targetBins,
                                 uint32_t schemaVersion)
{
    Fnv64 hash;
    hash.pod(schemaVersion);
    hash.pod(targetBins);
    hash.pod(snapshot.durationUs);

    const uint64_t sourceCount = static_cast<uint64_t>(snapshot.sources.size());
    hash.pod(sourceCount);
    for (size_t i = 0; i < snapshot.sources.size(); ++i)
    {
        const ADM_WaveformSourceSnapshot &source = snapshot.sources[i];
        hash.string(source.fileName);
        hash.pod(source.fileSize);
        hash.pod(source.modifiedTime);
    }

    const uint64_t segmentCount = static_cast<uint64_t>(snapshot.segments.size());
    hash.pod(segmentCount);
    for (size_t i = 0; i < snapshot.segments.size(); ++i)
    {
        const ADM_WaveformSegmentSnapshot &segment = snapshot.segments[i];
        hash.pod(segment.reference);
        hash.pod(segment.referenceStartUs);
        hash.pod(segment.timelineStartUs);
        hash.pod(segment.durationUs);
    }

    const uint64_t trackCount = static_cast<uint64_t>(snapshot.tracks.size());
    hash.pod(trackCount);
    for (size_t i = 0; i < snapshot.tracks.size(); ++i)
    {
        const ADM_WaveformTrackSnapshot &track = snapshot.tracks[i];
        const uint32_t sourceType = static_cast<uint32_t>(track.sourceType);
        hash.pod(sourceType);
        hash.pod(track.activeIndex);
        hash.pod(track.poolIndex);
        hash.pod(track.internalTrackIndex);
        hash.string(track.externalFileName);
        hash.pod(track.outputChannels);
        hash.pod(track.outputFrequency);
        hash.pod(track.externalFileSize);
        hash.pod(track.externalModifiedTime);
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << hash.value;
    return out.str();
}

bool ADM_writeWaveformCache(const std::string &path,
                            const std::string &cacheKey,
                            const ADM_WaveformCacheData &data)
{
    if (path.empty() || cacheKey.empty() || data.tracks.size() > kMaxTracks)
        return false;

    const std::string tempPath = path + ".tmp";
    FILE *out = ADM_fopen(tempPath.c_str(), "wb");
    if (!out)
        return false;

    bool ok = ADM_fwrite(kMagic, 1, sizeof(kMagic), out) == sizeof(kMagic) &&
              writePod(out, kFormatVersion) && writeString(out, cacheKey) &&
              writePod(out, data.durationUs);

    const uint32_t trackCount = static_cast<uint32_t>(data.tracks.size());
    ok = ok && writePod(out, trackCount);

    for (size_t track = 0; ok && track < data.tracks.size(); ++track)
    {
        if (data.tracks[track].size() > kMaxChannelsPerTrack)
        {
            ok = false;
            break;
        }
        const uint32_t channelCount = static_cast<uint32_t>(data.tracks[track].size());
        ok = writePod(out, channelCount);
        for (size_t channel = 0; ok && channel < data.tracks[track].size(); ++channel)
        {
            const std::vector<float> &peaks = data.tracks[track][channel];
            if (peaks.size() > kMaxBinsPerChannel)
            {
                ok = false;
                break;
            }
            const uint32_t peakCount = static_cast<uint32_t>(peaks.size());
            ok = writePod(out, peakCount);
            if (ok && peakCount)
                ok = ADM_fwrite(&peaks[0], sizeof(float), peakCount, out) == peakCount;
        }
    }

    if (ok)
        ok = fflush(out) == 0;
    ADM_fclose(out);

    if (!ok)
    {
        ADM_eraseFile(tempPath.c_str());
        return false;
    }

    ADM_eraseFile(path.c_str());
    if (!ADM_renameFile(tempPath.c_str(), path.c_str()))
    {
        ADM_eraseFile(tempPath.c_str());
        return false;
    }
    return true;
}

bool ADM_readWaveformCache(const std::string &path,
                           const std::string &expectedCacheKey,
                           ADM_WaveformCacheData *data)
{
    if (!data || path.empty() || expectedCacheKey.empty())
        return false;
    data->clear();

    FILE *in = ADM_fopen(path.c_str(), "rb");
    if (!in)
        return false;

    bool ok = true;
    char magic[sizeof(kMagic)] = {};
    ok = ADM_fread(magic, 1, sizeof(magic), in) == sizeof(magic) &&
         std::memcmp(magic, kMagic, sizeof(kMagic)) == 0;

    uint32_t version = 0;
    std::string cacheKey;
    uint32_t trackCount = 0;
    ok = ok && readPod(in, &version) && version == kFormatVersion &&
         readString(in, &cacheKey) && cacheKey == expectedCacheKey &&
         readPod(in, &data->durationUs) &&
         readPod(in, &trackCount) && trackCount <= kMaxTracks;

    if (ok)
        data->tracks.resize(trackCount);

    for (uint32_t track = 0; ok && track < trackCount; ++track)
    {
        uint32_t channelCount = 0;
        ok = readPod(in, &channelCount) && channelCount <= kMaxChannelsPerTrack;
        if (!ok)
            break;
        data->tracks[track].resize(channelCount);

        for (uint32_t channel = 0; ok && channel < channelCount; ++channel)
        {
            uint32_t peakCount = 0;
            ok = readPod(in, &peakCount) && peakCount <= kMaxBinsPerChannel;
            if (!ok)
                break;
            std::vector<float> &peaks = data->tracks[track][channel];
            peaks.resize(peakCount);
            if (peakCount)
                ok = ADM_fread(&peaks[0], sizeof(float), peakCount, in) == peakCount;
        }
    }

    ADM_fclose(in);
    if (!ok)
        data->clear();
    return ok;
}
