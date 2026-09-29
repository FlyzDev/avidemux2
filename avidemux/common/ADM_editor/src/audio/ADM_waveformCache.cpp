#include "ADM_waveformCache.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

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
bool writePod(std::ofstream &out, const T &value)
{
    out.write(reinterpret_cast<const char *>(&value), sizeof(value));
    return out.good();
}

template <typename T>
bool readPod(std::ifstream &in, T *value)
{
    if (!value)
        return false;
    in.read(reinterpret_cast<char *>(value), sizeof(*value));
    return in.good();
}

bool writeString(std::ofstream &out, const std::string &value)
{
    if (value.size() > kMaxKeyBytes)
        return false;
    const uint32_t size = static_cast<uint32_t>(value.size());
    if (!writePod(out, size))
        return false;
    if (size)
        out.write(value.data(), size);
    return out.good();
}

bool readString(std::ifstream &in, std::string *value)
{
    if (!value)
        return false;
    uint32_t size = 0;
    if (!readPod(in, &size) || size > kMaxKeyBytes)
        return false;
    value->assign(size, '\0');
    if (size)
        in.read(&(*value)[0], size);
    return in.good();
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
    std::ofstream out(tempPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    out.write(kMagic, sizeof(kMagic));
    if (!out.good() || !writePod(out, kFormatVersion) || !writeString(out, cacheKey) ||
        !writePod(out, data.durationUs))
    {
        out.close();
        std::remove(tempPath.c_str());
        return false;
    }

    const uint32_t trackCount = static_cast<uint32_t>(data.tracks.size());
    if (!writePod(out, trackCount))
        return false;

    for (size_t track = 0; track < data.tracks.size(); ++track)
    {
        if (data.tracks[track].size() > kMaxChannelsPerTrack)
            return false;
        const uint32_t channelCount = static_cast<uint32_t>(data.tracks[track].size());
        if (!writePod(out, channelCount))
            return false;

        for (size_t channel = 0; channel < data.tracks[track].size(); ++channel)
        {
            const std::vector<float> &peaks = data.tracks[track][channel];
            if (peaks.size() > kMaxBinsPerChannel)
                return false;
            const uint32_t peakCount = static_cast<uint32_t>(peaks.size());
            if (!writePod(out, peakCount))
                return false;
            if (peakCount)
                out.write(reinterpret_cast<const char *>(&peaks[0]), peakCount * sizeof(float));
            if (!out.good())
                return false;
        }
    }

    out.flush();
    const bool ok = out.good();
    out.close();
    if (!ok)
    {
        std::remove(tempPath.c_str());
        return false;
    }

    std::remove(path.c_str());
    if (std::rename(tempPath.c_str(), path.c_str()) != 0)
    {
        std::remove(tempPath.c_str());
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

    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in)
        return false;

    char magic[sizeof(kMagic)] = {};
    in.read(magic, sizeof(magic));
    if (!in.good() || std::memcmp(magic, kMagic, sizeof(kMagic)) != 0)
        return false;

    uint32_t version = 0;
    std::string cacheKey;
    uint32_t trackCount = 0;
    if (!readPod(in, &version) || version != kFormatVersion ||
        !readString(in, &cacheKey) || cacheKey != expectedCacheKey ||
        !readPod(in, &data->durationUs) ||
        !readPod(in, &trackCount) || trackCount > kMaxTracks)
    {
        data->clear();
        return false;
    }

    data->tracks.resize(trackCount);
    for (uint32_t track = 0; track < trackCount; ++track)
    {
        uint32_t channelCount = 0;
        if (!readPod(in, &channelCount) || channelCount > kMaxChannelsPerTrack)
        {
            data->clear();
            return false;
        }
        data->tracks[track].resize(channelCount);

        for (uint32_t channel = 0; channel < channelCount; ++channel)
        {
            uint32_t peakCount = 0;
            if (!readPod(in, &peakCount) || peakCount > kMaxBinsPerChannel)
            {
                data->clear();
                return false;
            }
            std::vector<float> &peaks = data->tracks[track][channel];
            peaks.resize(peakCount);
            if (peakCount)
                in.read(reinterpret_cast<char *>(&peaks[0]), peakCount * sizeof(float));
            if (!in.good())
            {
                data->clear();
                return false;
            }
        }
    }

    return true;
}
