#pragma once

#include <QWidget>
#include <stdint.h>
#include <vector>

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
        DisplayTracks = 1
    };

    explicit ADM_mwWaveform(QWidget *parent = NULL);
    virtual ~ADM_mwWaveform();

    void setDuration(uint64_t duration);
    void setPosition(uint64_t position);
    void setMarkers(uint64_t markerA, uint64_t markerB);
    void setTrackCount(int tracks);
    void setDisplayMode(DisplayMode mode);
    DisplayMode displayMode(void) const { return mode; }

    void clearPeaks(void);
    void setCombinedPeaks(const std::vector<float> &peaks);
    void setTrackPeaks(const std::vector<std::vector<float> > &peaks);

    QSize sizeHint(void) const override;
    QSize minimumSizeHint(void) const override;

signals:
    void seekRequested(double ratio);

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
    std::vector<float> combinedPeaks;
    std::vector<std::vector<float> > trackPeaks;

    int timeToX(uint64_t time) const;
    void updatePreferredHeight(void);
    void drawPeakVector(QPainter &painter, const QRect &rect, const std::vector<float> &peaks) const;
    void drawEmptyTrack(QPainter &painter, const QRect &rect, const QString &label) const;
};
