#include "ADM_waveformSnapshot.h"

#include "ADM_edit.hxx"
#include "ADM_edActiveAudioTracks.h"
#include "ADM_edAudioTrackExternal.h"
#include "ADM_edAudioTrackFromVideo.h"
#include "ADM_edEditableAudioTrack.h"
#include "ADM_segment.h"

bool ADM_buildWaveformSnapshot(ADM_Composer *composer, ADM_WaveformSnapshot *snapshot)
{
    if (!composer || !snapshot || !composer->isFileOpen())
        return false;

    snapshot->clear();
    snapshot->durationUs = composer->getVideoDuration();

    const int sourceCount = composer->getVideoCount();
    snapshot->sources.reserve(sourceCount > 0 ? static_cast<size_t>(sourceCount) : 0);
    for (int sourceIndex = 0; sourceIndex < sourceCount; ++sourceIndex)
    {
        _VIDEOS *video = composer->getRefVideo(sourceIndex);
        if (!video || !video->_aviheader)
        {
            snapshot->clear();
            return false;
        }

        ADM_WaveformSourceSnapshot source;
        const char *name = video->_aviheader->getMyName();
        if (name)
            source.fileName = name;
        snapshot->sources.push_back(source);
    }

    const uint32_t segmentCount = composer->getNbSegment();
    snapshot->segments.reserve(segmentCount);
    for (uint32_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
    {
        _SEGMENT *segment = composer->getSegment(segmentIndex);
        if (!segment || segment->_reference >= snapshot->sources.size())
        {
            snapshot->clear();
            return false;
        }

        ADM_WaveformSegmentSnapshot item;
        item.reference = segment->_reference;
        item.referenceStartUs = segment->_refStartTimeUs;
        item.timelineStartUs = segment->_startTimeUs;
        item.durationUs = segment->_durationUs;
        snapshot->segments.push_back(item);
    }

    ActiveAudioTracks *active = composer->getPoolOfActiveAudioTrack();
    if (!active)
        return true;

    snapshot->tracks.reserve(active->size());
    for (unsigned int activeIndex = 0; activeIndex < active->size(); ++activeIndex)
    {
        EditableAudioTrack *editable = active->atEditable(activeIndex);
        ADM_edAudioTrack *track = editable ? editable->edTrack : NULL;
        if (!editable || !track)
            continue;

        ADM_WaveformTrackSnapshot item;
        item.activeIndex = static_cast<int>(activeIndex);
        item.poolIndex = editable->poolIndex;
        item.internalTrackIndex = -1;
        item.outputChannels = track->getOutputChannels();
        item.outputFrequency = track->getOutputFrequency();
        item.externalFileName.clear();

        if (track->getTrackType() == ADM_EDAUDIO_FROM_VIDEO)
        {
            ADM_edAudioTrackFromVideo *internal = track->castToTrackFromVideo();
            if (!internal)
                continue;
            item.sourceType = ADM_WAVEFORM_TRACK_INTERNAL;
            item.internalTrackIndex = internal->getMyTrackIndex();
        }
        else if (track->getTrackType() == ADM_EDAUDIO_EXTERNAL)
        {
            ADM_edAudioTrackExternal *external = track->castToExternal();
            if (!external)
                continue;
            item.sourceType = ADM_WAVEFORM_TRACK_EXTERNAL;
            item.externalFileName = external->getMyName();
        }
        else
        {
            continue;
        }

        snapshot->tracks.push_back(item);
    }

    return true;
}
