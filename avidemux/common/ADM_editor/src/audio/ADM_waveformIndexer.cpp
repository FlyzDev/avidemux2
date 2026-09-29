#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>
#include <vector>

#include "ADM_waveformIndexer.h"

#include "ADM_Video.h"
#include "ADM_audiocodec.h"
#include "ADM_audiodef.h"
#include "ADM_audioStream.h"
#include "ADM_coreDemuxer.h"
#include "ADM_default.h"
#include "ADM_edAudioTrackExternal.h"
#include "ADM_waveformPeak.h"

namespace
{
static const uint32_t kPacketBytes = 64 * 1024;
static const uint32_t kMaxEmptyPackets = 128;

bool cancelled(ADM_WaveformCancelFn fn, void *opaque)
{
    return fn && fn(opaque);
}

void setError(std::string *target, const std::string &message)
{
    if (target)
        *target = message;
}

bool readMagic(const std::string &path, uint32_t *magic)
{
    if (!magic)
        return false;
    unsigned char bytes[4] = {0, 0, 0, 0};
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in)
        return false;
    in.read(reinterpret_cast<char *>(bytes), sizeof(bytes));
    if (in.gcount() != static_cast<std::streamsize>(sizeof(bytes)))
        return false;
    *magic = (static_cast<uint32_t>(bytes[3]) << 24) |
             (static_cast<uint32_t>(bytes[2]) << 16) |
             (static_cast<uint32_t>(bytes[1]) << 8) |
              static_cast<uint32_t>(bytes[0]);
    return true;
}

class IndependentInternalDecoder
{
public:
    IndependentInternalDecoder()
        : demuxer(NULL), stream(NULL), codec(NULL), outputChannels(0), outputFrequency(0),
          packet(kPacketBytes), pcm(static_cast<size_t>(MAX_SAMPLING_RATE) * MAX_CHANNELS)
    {
    }

    ~IndependentInternalDecoder()
    {
        if (codec)
            delete codec;
        codec = NULL;
        stream = NULL; // owned by demuxer
        if (demuxer)
        {
            demuxer->close();
            delete demuxer;
        }
        demuxer = NULL;
    }

    bool open(const std::string &path, int trackIndex, std::string *error)
    {
        if (path.empty() || trackIndex < 0)
        {
            setError(error, "Invalid source path or track index");
            return false;
        }

        uint32_t magic = 0;
        if (!readMagic(path, &magic))
        {
            setError(error, "Cannot read source header: " + path);
            return false;
        }

        demuxer = ADM_demuxerSpawn(magic, path.c_str());
        if (!demuxer)
        {
            setError(error, "Cannot find demuxer for waveform source: " + path);
            return false;
        }
        if (!demuxer->open(path.c_str()))
        {
            setError(error, "Cannot open waveform source: " + path);
            return false;
        }

        const uint32_t audioCount = demuxer->getNbAudioStreams();
        if (static_cast<uint32_t>(trackIndex) >= audioCount)
        {
            setError(error, "Audio track index is not present in source: " + path);
            return false;
        }

        WAVHeader *header = demuxer->getAudioInfo(static_cast<uint32_t>(trackIndex));
        if (!header || !demuxer->getAudioStream(static_cast<uint32_t>(trackIndex), &stream) || !stream)
        {
            setError(error, "Cannot open audio stream for waveform source: " + path);
            return false;
        }

        WAVHeader decoderHeader = *header;
        if (decoderHeader.encoding == 0x706d)
            decoderHeader.encoding = WAV_AAC;

        uint32_t extraLen = 0;
        uint8_t *extraData = NULL;
        stream->getExtraData(&extraLen, &extraData);
        codec = getAudioCodec(decoderHeader.encoding, &decoderHeader, extraLen, extraData);
        if (!codec || codec->isDummy())
        {
            setError(error, "No audio decoder available for waveform source: " + path);
            return false;
        }

        outputChannels = codec->getOutputChannels();
        outputFrequency = codec->getOutputFrequency();
        if (!outputChannels || outputChannels > MAX_CHANNELS ||
            outputFrequency < MIN_SAMPLING_RATE || outputFrequency > MAX_SAMPLING_RATE)
        {
            setError(error, "Invalid decoded audio format for waveform source: " + path);
            return false;
        }
        return true;
    }

    bool seek(uint64_t timeUs)
    {
        if (!stream || !codec)
            return false;
        codec->resetAfterSeek();
        return stream->goToTime(timeUs);
    }

    bool next(uint64_t *dts, uint32_t *frames, const float **samples)
    {
        if (!stream || !codec || !dts || !frames || !samples)
            return false;

        *frames = 0;
        *samples = NULL;
        uint32_t packetBytes = 0;
        uint32_t packetSamples = 0;
        uint64_t packetDts = ADM_AUDIO_NO_DTS;
        if (!stream->getPacket(&packet[0], &packetBytes, static_cast<uint32_t>(packet.size()),
                               &packetSamples, &packetDts))
            return false;
        if (!packetBytes)
            return true;

        uint32_t outputValues = 0;
        if (!codec->run(&packet[0], packetBytes, &pcm[0], &outputValues))
            return true;

        const uint32_t currentChannels = codec->getOutputChannels();
        const uint32_t currentFrequency = codec->getOutputFrequency();
        if (!currentChannels || currentChannels > MAX_CHANNELS ||
            !currentFrequency || currentFrequency > MAX_SAMPLING_RATE)
            return false;
        if (currentChannels != outputChannels || currentFrequency != outputFrequency)
        {
            outputChannels = currentChannels;
            outputFrequency = currentFrequency;
        }

        *frames = outputValues / outputChannels;
        *dts = packetDts;
        *samples = &pcm[0];
        return true;
    }

    uint32_t channels(void) const { return outputChannels; }
    uint32_t frequency(void) const { return outputFrequency; }

private:
    vidHeader *demuxer;
    ADM_audioStream *stream;
    ADM_Audiocodec *codec;
    uint32_t outputChannels;
    uint32_t outputFrequency;
    std::vector<uint8_t> packet;
    std::vector<float> pcm;
};

uint64_t framesToUs(uint64_t frames, uint32_t frequency)
{
    if (!frequency)
        return 0;
    return static_cast<uint64_t>((static_cast<long double>(frames) * 1000000.0L) /
                                 static_cast<long double>(frequency));
}

bool decodeInternalSegment(IndependentInternalDecoder &decoder,
                           const ADM_WaveformSegmentSnapshot &segment,
                           ADM_WaveformPeakAccumulator *accumulator,
                           ADM_WaveformCancelFn cancel,
                           void *cancelOpaque,
                           std::string *error)
{
    if (!accumulator || !decoder.seek(segment.referenceStartUs))
    {
        setError(error, "Cannot seek audio while indexing waveform");
        return false;
    }

    const uint64_t referenceEnd = segment.referenceStartUs + segment.durationUs;
    uint64_t fallbackDts = segment.referenceStartUs;
    uint32_t emptyPackets = 0;

    while (!cancelled(cancel, cancelOpaque))
    {
        uint64_t packetDts = ADM_AUDIO_NO_DTS;
        uint32_t frames = 0;
        const float *pcm = NULL;
        if (!decoder.next(&packetDts, &frames, &pcm))
            break;
        if (!frames || !pcm)
        {
            if (++emptyPackets >= kMaxEmptyPackets)
                break;
            continue;
        }
        emptyPackets = 0;

        const uint32_t channels = decoder.channels();
        const uint32_t frequency = decoder.frequency();
        if (channels != accumulator->channels() || frequency != accumulator->sampleRate())
        {
            setError(error, "Decoded audio format changed during waveform indexing");
            return false;
        }

        uint64_t startUs = packetDts == ADM_AUDIO_NO_DTS ? fallbackDts : packetDts;
        uint64_t endUs = startUs + framesToUs(frames, frequency);
        fallbackDts = endUs;

        if (endUs <= segment.referenceStartUs)
            continue;
        if (startUs >= referenceEnd)
            break;

        uint32_t firstFrame = 0;
        if (startUs < segment.referenceStartUs)
        {
            const uint64_t deltaUs = segment.referenceStartUs - startUs;
            firstFrame = static_cast<uint32_t>(std::min<uint64_t>(frames,
                (deltaUs * frequency + 999999ULL) / 1000000ULL));
        }

        uint32_t usableFrames = frames - firstFrame;
        const uint64_t clippedStartUs = startUs + framesToUs(firstFrame, frequency);
        if (clippedStartUs >= referenceEnd)
            break;

        const uint64_t availableUs = referenceEnd - clippedStartUs;
        const uint64_t maxFrames = (availableUs * frequency + 999999ULL) / 1000000ULL;
        if (usableFrames > maxFrames)
            usableFrames = static_cast<uint32_t>(maxFrames);
        if (!usableFrames)
            continue;

        const uint64_t relativeUs = clippedStartUs > segment.referenceStartUs
                                  ? clippedStartUs - segment.referenceStartUs : 0;
        const uint64_t timelineUs = segment.timelineStartUs + relativeUs;
        accumulator->addInterleaved(pcm + static_cast<size_t>(firstFrame) * channels,
                                    usableFrames, timelineUs);

        if (endUs >= referenceEnd)
            break;
    }

    if (cancelled(cancel, cancelOpaque))
    {
        setError(error, "Waveform indexing cancelled");
        return false;
    }
    return true;
}

bool decodeInternalTrack(const ADM_WaveformSnapshot &snapshot,
                         const ADM_WaveformTrackSnapshot &track,
                         uint32_t targetBins,
                         std::vector<std::vector<float> > *output,
                         ADM_WaveformCancelFn cancel,
                         void *cancelOpaque,
                         std::string *error)
{
    if (!output || !track.outputChannels || !track.outputFrequency)
        return false;

    ADM_WaveformPeakAccumulator accumulator(track.outputFrequency, track.outputChannels,
                                            snapshot.durationUs, targetBins);
    if (!accumulator.valid())
    {
        setError(error, "Cannot initialize waveform peak accumulator");
        return false;
    }

    std::vector<std::unique_ptr<IndependentInternalDecoder> > decoders(snapshot.sources.size());
    for (size_t segmentIndex = 0; segmentIndex < snapshot.segments.size(); ++segmentIndex)
    {
        if (cancelled(cancel, cancelOpaque))
        {
            setError(error, "Waveform indexing cancelled");
            return false;
        }

        const ADM_WaveformSegmentSnapshot &segment = snapshot.segments[segmentIndex];
        if (segment.reference >= snapshot.sources.size())
        {
            setError(error, "Waveform segment references a missing source");
            return false;
        }
        if (!segment.durationUs)
            continue;

        std::unique_ptr<IndependentInternalDecoder> &decoder = decoders[segment.reference];
        if (!decoder.get())
        {
            decoder.reset(new IndependentInternalDecoder());
            if (!decoder->open(snapshot.sources[segment.reference].fileName,
                               track.internalTrackIndex, error))
                return false;
        }
        // Some codecs (notably AAC with SBR) only reveal their final output
        // frequency / channel layout after decoding the first packet. Validate
        // against the accumulator inside decodeInternalSegment after run().
        if (!decodeInternalSegment(*decoder, segment, &accumulator, cancel, cancelOpaque, error))
            return false;
    }

    *output = accumulator.channelPeaks();
    return true;
}

bool decodeExternalTrack(const ADM_WaveformSnapshot &snapshot,
                         const ADM_WaveformTrackSnapshot &track,
                         uint32_t targetBins,
                         std::vector<std::vector<float> > *output,
                         ADM_WaveformCancelFn cancel,
                         void *cancelOpaque,
                         std::string *error)
{
    if (!output || track.externalFileName.empty())
        return false;

    std::unique_ptr<ADM_edAudioTrackExternal> decoder(create_edAudioExternal(track.externalFileName.c_str()));
    if (!decoder.get())
    {
        setError(error, "Cannot open external audio track for waveform indexing");
        return false;
    }

    const uint32_t channels = decoder->getOutputChannels();
    const uint32_t frequency = decoder->getOutputFrequency();
    if (!channels || !frequency || channels != track.outputChannels || frequency != track.outputFrequency)
    {
        setError(error, "External audio layout differs from active track layout");
        return false;
    }

    ADM_WaveformPeakAccumulator accumulator(frequency, channels, snapshot.durationUs, targetBins);
    if (!accumulator.valid() || !decoder->goToTime(0))
    {
        setError(error, "Cannot initialize external waveform decoder");
        return false;
    }

    std::vector<float> pcm(static_cast<size_t>(MAX_SAMPLING_RATE) * MAX_CHANNELS);
    uint32_t emptyPackets = 0;
    uint64_t fallbackDts = 0;
    while (!cancelled(cancel, cancelOpaque))
    {
        uint32_t frames = 0;
        uint64_t dts = ADM_AUDIO_NO_DTS;
        if (!decoder->getPCMPacket(&pcm[0], static_cast<uint32_t>(pcm.size()), &frames, &dts))
            break;
        if (!frames)
        {
            if (++emptyPackets >= kMaxEmptyPackets)
                break;
            continue;
        }
        emptyPackets = 0;
        const uint64_t startUs = dts == ADM_AUDIO_NO_DTS ? fallbackDts : dts;
        if (startUs >= snapshot.durationUs)
            break;
        uint32_t usableFrames = frames;
        const uint64_t availableUs = snapshot.durationUs - startUs;
        const uint64_t maxFrames = (availableUs * frequency + 999999ULL) / 1000000ULL;
        if (usableFrames > maxFrames)
            usableFrames = static_cast<uint32_t>(maxFrames);
        accumulator.addInterleaved(&pcm[0], usableFrames, startUs);
        fallbackDts = startUs + framesToUs(frames, frequency);
    }

    if (cancelled(cancel, cancelOpaque))
    {
        setError(error, "Waveform indexing cancelled");
        return false;
    }

    *output = accumulator.channelPeaks();
    return true;
}
}

bool ADM_generateWaveform(const ADM_WaveformSnapshot &snapshot,
                          uint32_t targetBins,
                          ADM_WaveformCacheData *output,
                          std::string *errorMessage,
                          ADM_WaveformCancelFn cancel,
                          void *cancelOpaque)
{
    if (!output || !targetBins || !snapshot.durationUs)
    {
        setError(errorMessage, "Invalid waveform indexing request");
        return false;
    }

    output->clear();
    output->durationUs = snapshot.durationUs;
    output->tracks.resize(snapshot.tracks.size());

    for (size_t trackIndex = 0; trackIndex < snapshot.tracks.size(); ++trackIndex)
    {
        if (cancelled(cancel, cancelOpaque))
        {
            output->clear();
            setError(errorMessage, "Waveform indexing cancelled");
            return false;
        }

        const ADM_WaveformTrackSnapshot &track = snapshot.tracks[trackIndex];
        bool ok = false;
        if (track.sourceType == ADM_WAVEFORM_TRACK_INTERNAL)
            ok = decodeInternalTrack(snapshot, track, targetBins, &output->tracks[trackIndex],
                                     cancel, cancelOpaque, errorMessage);
        else if (track.sourceType == ADM_WAVEFORM_TRACK_EXTERNAL)
            ok = decodeExternalTrack(snapshot, track, targetBins, &output->tracks[trackIndex],
                                     cancel, cancelOpaque, errorMessage);
        if (!ok)
        {
            output->clear();
            return false;
        }
    }
    return true;
}
