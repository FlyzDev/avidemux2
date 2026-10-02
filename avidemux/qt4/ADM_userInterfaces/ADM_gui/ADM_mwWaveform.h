#pragma once

#include <QWidget>
#include <stdint.h>
#include <vector>
#include "ADM_timelineMarker.h"

class QPainter;
class QMouseEvent;
class QContextMenuEvent;

class ADM_mwWaveform : public QWidget
{
    Q_OBJECT
public:
    enum DisplayMode
    {
        DisplayCombined = 0,
        DisplayTracks = 1,
        DisplayChannels = 2
    };

    explicit ADM_mwWaveform(QWidget *parent = NULL);
    virtual ~ADM_mwWaveform();

    void setDuration(uint64_t duration);
    void setPosition(uint64_t position);
    void setMarkers(uint64_t markerA, uint64_t markerB);
    void setTrackCount(int tracks);
    void setDisplayMode(DisplayMode mode);
    DisplayMode displayMode(void) const { return mode; }
    void setGenerating(bool active);
    void setTimelineMarkers(const std::vector<ADM_TimelineMarker> &markers);
    void clearTimelineMarkers(void);

    void clearPeaks(void);
    void setCombinedPeaks(const std::vector<float> &peaks);
    void setTrackPeaks(const std::vector<std::vector<float> > &peaks);
    void setChannelPeaks(const std::vector<std::vector<std::vector<float> > > &peaks);

    QSize sizeHint(void) const override;
    QSize minimumSizeHint(void) const override;

signals:
    void seekRequested(double ratio);
    void preferredHeightChanged(int height);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    uint64_t totalDuration;
    uint64_t currentPosition;
    uint64_t markerATime;
    uint64_t markerBTime;
    int trackCount;
    DisplayMode mode;
    bool generating;
    std::vector<ADM_TimelineMarker> timelineMarkers;
    std::vector<float> combinedPeaks;
    std::vector<std::vector<float> > trackPeaks;
    std::vector<std::vector<std::vector<float> > > channelPeaks;
    float combinedDisplayReference;
    float trackDisplayReference;
    float channelDisplayReference;

    int timeToX(uint64_t time) const;
    int channelRows(void) const;
    void updatePreferredHeight(void);
    void updateDisplayReferences(void);
    void drawPeakVector(QPainter &painter, const QRect &rect, const std::vector<float> &peaks,
                        float displayReference) const;
    void drawLaneLabel(QPainter &painter, const QRect &rect, const QString &label) const;
    void drawEmptyTrack(QPainter &painter, const QRect &rect, const QString &label) const;
    void drawTimelineMarkers(QPainter &painter) const;
    void rebuildCombinedPeaks(void);
    void rebuildTrackPeaksFromChannels(void);
};
