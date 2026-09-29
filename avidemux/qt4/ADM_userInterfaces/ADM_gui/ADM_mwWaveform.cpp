#include "ADM_mwWaveform.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QSizePolicy>

#include <algorithm>
#include <cmath>

ADM_mwWaveform::ADM_mwWaveform(QWidget *parent)
    : QWidget(parent),
      totalDuration(0),
      currentPosition(0),
      markerATime(0),
      markerBTime(0),
      trackCount(0),
      mode(DisplayCombined)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(52);
    setToolTip(tr("Audio waveform. A1/master is shown by default. Left-click to seek, right-click to change waveform view."));
}

ADM_mwWaveform::~ADM_mwWaveform()
{
}

QSize ADM_mwWaveform::sizeHint(void) const
{
    return QSize(480, minimumHeight());
}

QSize ADM_mwWaveform::minimumSizeHint(void) const
{
    return QSize(120, minimumHeight());
}

void ADM_mwWaveform::setDuration(uint64_t duration)
{
    if (totalDuration == duration)
        return;
    totalDuration = duration;
    if (currentPosition > totalDuration)
        currentPosition = totalDuration;
    update();
}

void ADM_mwWaveform::setPosition(uint64_t position)
{
    if (totalDuration && position > totalDuration)
        position = totalDuration;
    if (currentPosition == position)
        return;
    currentPosition = position;
    update();
}

void ADM_mwWaveform::setMarkers(uint64_t markerA, uint64_t markerB)
{
    markerATime = markerA;
    markerBTime = markerB;
    update();
}

void ADM_mwWaveform::setTrackCount(int tracks)
{
    if (tracks < 0)
        tracks = 0;
    if (trackCount == tracks)
        return;
    trackCount = tracks;
    if (trackCount < 2 && mode == DisplayTracks)
        mode = DisplayCombined;
    if (mode == DisplayChannels && channelRows() == 0)
        mode = DisplayCombined;
    updatePreferredHeight();
    update();
}

void ADM_mwWaveform::setDisplayMode(DisplayMode newMode)
{
    if (newMode == DisplayTracks && trackCount < 2)
        newMode = DisplayCombined;
    if (newMode == DisplayChannels && channelRows() == 0)
        newMode = DisplayCombined;
    if (mode == newMode)
        return;
    mode = newMode;
    updatePreferredHeight();
    update();
}

void ADM_mwWaveform::clearPeaks(void)
{
    combinedPeaks.clear();
    trackPeaks.clear();
    channelPeaks.clear();
    updatePreferredHeight();
    update();
}

void ADM_mwWaveform::setCombinedPeaks(const std::vector<float> &peaks)
{
    combinedPeaks = peaks;
    update();
}

void ADM_mwWaveform::setTrackPeaks(const std::vector<std::vector<float> > &peaks)
{
    channelPeaks.clear();
    trackPeaks = peaks;
    if (trackCount != static_cast<int>(trackPeaks.size()))
        trackCount = static_cast<int>(trackPeaks.size());
    rebuildCombinedPeaks();
    if (mode == DisplayChannels)
        mode = DisplayCombined;
    updatePreferredHeight();
    update();
}

void ADM_mwWaveform::setChannelPeaks(const std::vector<std::vector<std::vector<float> > > &peaks)
{
    channelPeaks = peaks;
    trackCount = static_cast<int>(channelPeaks.size());
    rebuildTrackPeaksFromChannels();
    rebuildCombinedPeaks();
    updatePreferredHeight();
    update();
}

void ADM_mwWaveform::rebuildTrackPeaksFromChannels(void)
{
    trackPeaks.assign(channelPeaks.size(), std::vector<float>());
    for (size_t track = 0; track < channelPeaks.size(); ++track)
    {
        size_t bins = 0;
        for (size_t channel = 0; channel < channelPeaks[track].size(); ++channel)
            bins = std::max(bins, channelPeaks[track][channel].size());
        trackPeaks[track].assign(bins, 0.0f);
        if (!bins)
            continue;
        for (size_t channel = 0; channel < channelPeaks[track].size(); ++channel)
        {
            const std::vector<float> &source = channelPeaks[track][channel];
            if (source.empty())
                continue;
            for (size_t bin = 0; bin < bins; ++bin)
            {
                const size_t sourceIndex = std::min(source.size() - 1, (bin * source.size()) / bins);
                trackPeaks[track][bin] = std::max(trackPeaks[track][bin], std::fabs(source[sourceIndex]));
            }
        }
    }
}

void ADM_mwWaveform::rebuildCombinedPeaks(void)
{
    combinedPeaks.clear();
    if (trackPeaks.empty())
        return;

    // Track A1 is treated as the default / master waveform. This matches common
    // multi-track capture layouts (for example OBS master mix on A1 with isolated
    // mic / desktop tracks following it) and avoids inventing a mix across
    // independent container audio streams.
    combinedPeaks = trackPeaks.front();
}

int ADM_mwWaveform::timeToX(uint64_t time) const
{
    if (!totalDuration || width() <= 1)
        return 0;
    if (time > totalDuration)
        time = totalDuration;
    return static_cast<int>((static_cast<double>(time) / static_cast<double>(totalDuration)) * (width() - 1));
}

int ADM_mwWaveform::channelRows(void) const
{
    int rows = 0;
    for (size_t track = 0; track < channelPeaks.size(); ++track)
        rows += static_cast<int>(channelPeaks[track].size());
    return rows;
}

void ADM_mwWaveform::updatePreferredHeight(void)
{
    int wanted = 52;
    if (mode == DisplayTracks && trackCount > 1)
        wanted = std::min(180, std::max(64, trackCount * 34));
    else if (mode == DisplayChannels && channelRows() > 0)
        wanted = std::min(240, std::max(72, channelRows() * 30));
    setMinimumHeight(wanted);
    updateGeometry();
}

void ADM_mwWaveform::drawPeakVector(QPainter &painter, const QRect &rect, const std::vector<float> &peaks) const
{
    if (rect.width() <= 0 || rect.height() <= 0)
        return;

    const int center = rect.center().y();
    QColor baseline = palette().color(QPalette::Mid);
    baseline.setAlpha(110);
    painter.setPen(baseline);
    painter.drawLine(rect.left(), center, rect.right(), center);

    if (peaks.empty())
        return;

    QColor wave = palette().color(QPalette::Highlight);
    painter.setPen(wave);
    const int halfHeight = std::max(1, rect.height() / 2 - 3);
    const size_t peakCount = peaks.size();

    for (int x = 0; x < rect.width(); ++x)
    {
        const size_t index = std::min(peakCount - 1,
                                      static_cast<size_t>((static_cast<double>(x) / std::max(1, rect.width() - 1)) * (peakCount - 1)));
        const float amplitude = std::min(1.0f, std::fabs(peaks[index]));
        const int extent = static_cast<int>(amplitude * halfHeight);
        painter.drawLine(rect.left() + x, center - extent, rect.left() + x, center + extent);
    }
}

void ADM_mwWaveform::drawLaneLabel(QPainter &painter, const QRect &rect, const QString &label) const
{
    if (label.isEmpty())
        return;
    const QFontMetrics metrics = painter.fontMetrics();
    const int boxWidth = metrics.horizontalAdvance(label) + 10;
    const int boxHeight = std::min(rect.height(), metrics.height() + 4);
    QRect box(rect.left() + 3, rect.top() + 2, boxWidth, boxHeight);

    QColor background = palette().color(QPalette::Base);
    background.setAlpha(190);
    painter.fillRect(box, background);

    QColor text = palette().color(QPalette::Text);
    text.setAlpha(190);
    painter.setPen(text);
    painter.drawText(box.adjusted(5, 0, -3, 0), Qt::AlignLeft | Qt::AlignVCenter, label);
}

void ADM_mwWaveform::drawEmptyTrack(QPainter &painter, const QRect &rect, const QString &label) const
{
    drawPeakVector(painter, rect, std::vector<float>());
    drawLaneLabel(painter, rect, label);
}

void ADM_mwWaveform::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    if (totalDuration && markerBTime >= markerATime)
    {
        const int a = timeToX(markerATime);
        const int b = timeToX(markerBTime);
        QColor selection = palette().color(QPalette::Highlight);
        selection.setAlpha(32);
        painter.fillRect(QRect(QPoint(a, 0), QPoint(b, height() - 1)).normalized(), selection);
    }

    const QRect content = rect().adjusted(0, 2, 0, -2);
    if (trackCount <= 0)
    {
        drawEmptyTrack(painter, content, tr("No audio"));
    }
    else if (mode == DisplayTracks && trackCount > 1)
    {
        const int rowHeight = std::max(1, content.height() / trackCount);
        for (int i = 0; i < trackCount; ++i)
        {
            const int top = content.top() + i * rowHeight;
            const int bottom = (i == trackCount - 1) ? content.bottom() : top + rowHeight - 1;
            QRect row(content.left(), top, content.width(), bottom - top + 1);
            if (i > 0)
            {
                QColor divider = palette().color(QPalette::Mid);
                divider.setAlpha(70);
                painter.setPen(divider);
                painter.drawLine(row.left(), row.top(), row.right(), row.top());
            }
            const QString label = tr("A%1").arg(i + 1);
            if (i < static_cast<int>(trackPeaks.size()) && !trackPeaks[i].empty())
            {
                drawPeakVector(painter, row, trackPeaks[i]);
                drawLaneLabel(painter, row, label);
            }
            else
                drawEmptyTrack(painter, row, label);
        }
    }
    else if (mode == DisplayChannels && channelRows() > 0)
    {
        const int rows = channelRows();
        const int rowHeight = std::max(1, content.height() / rows);
        int rowIndex = 0;
        for (size_t track = 0; track < channelPeaks.size(); ++track)
        {
            for (size_t channel = 0; channel < channelPeaks[track].size(); ++channel, ++rowIndex)
            {
                const int top = content.top() + rowIndex * rowHeight;
                const int bottom = (rowIndex == rows - 1) ? content.bottom() : top + rowHeight - 1;
                QRect row(content.left(), top, content.width(), bottom - top + 1);
                if (rowIndex > 0)
                {
                    QColor divider = palette().color(QPalette::Mid);
                    divider.setAlpha(70);
                    painter.setPen(divider);
                    painter.drawLine(row.left(), row.top(), row.right(), row.top());
                }
                const QString label = tr("A%1 C%2").arg(track + 1).arg(channel + 1);
                const std::vector<float> &peaks = channelPeaks[track][channel];
                if (!peaks.empty())
                {
                    drawPeakVector(painter, row, peaks);
                    drawLaneLabel(painter, row, label);
                }
                else
                    drawEmptyTrack(painter, row, label);
            }
        }
    }
    else
    {
        if (!combinedPeaks.empty())
            drawPeakVector(painter, content, combinedPeaks);
        else
            drawEmptyTrack(painter, content, tr("Master waveform (A1)"));
    }

    if (totalDuration)
    {
        QColor marker = palette().color(QPalette::Highlight);
        marker.setAlpha(190);
        painter.setPen(marker);
        painter.drawLine(timeToX(markerATime), 0, timeToX(markerATime), height() - 1);
        painter.drawLine(timeToX(markerBTime), 0, timeToX(markerBTime), height() - 1);

        QColor playhead = palette().color(QPalette::Text);
        painter.setPen(QPen(playhead, 2));
        painter.drawLine(timeToX(currentPosition), 0, timeToX(currentPosition), height() - 1);
    }
}

void ADM_mwWaveform::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && width() > 1)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const double x = event->position().x();
#else
        const double x = event->pos().x();
#endif
        double ratio = x / static_cast<double>(width() - 1);
        ratio = std::max(0.0, std::min(1.0, ratio));
        emit seekRequested(ratio);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ADM_mwWaveform::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *combined = menu.addAction(tr("Master waveform (A1)"));
    QAction *tracks = menu.addAction(tr("Separate audio tracks"));
    QAction *channels = menu.addAction(tr("Separate channels"));
    combined->setCheckable(true);
    tracks->setCheckable(true);
    channels->setCheckable(true);
    combined->setChecked(mode == DisplayCombined);
    tracks->setChecked(mode == DisplayTracks);
    channels->setChecked(mode == DisplayChannels);
    tracks->setEnabled(trackCount > 1);
    channels->setEnabled(channelRows() > 0);

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == combined)
        setDisplayMode(DisplayCombined);
    else if (chosen == tracks)
        setDisplayMode(DisplayTracks);
    else if (chosen == channels)
        setDisplayMode(DisplayChannels);
}
