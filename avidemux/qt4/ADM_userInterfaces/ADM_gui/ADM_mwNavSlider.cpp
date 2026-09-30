
/***************************************************************************
Custom slider
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include "ADM_inttype.h"
#include <QPainter>
#include <QPolygon>
#include <QSlider>
#include <QWheelEvent>
#include "ADM_mwNavSlider.h"
#include "ADM_assert.h"
#include "ADM_default.h"
/**
    \fn ADM_QSlider
*/
ADM_mwNavSlider::ADM_mwNavSlider(QWidget *parent) : ADM_flyNavSlider(parent)
{
    segments = NULL;
    numOfSegments = 0;
}
/**
    \fn dtor
*/
ADM_mwNavSlider::~ADM_mwNavSlider()
{
    if (segments)
        delete [] segments;
    segments = NULL;
}
/**
    \fn drawCutPoints
*/
void ADM_mwNavSlider::drawCutPoints(void)
{
    if (!segments || numOfSegments < 2 || !totalDuration)
        return;

    int pos, prevpos;
    prevpos = -1;
    QPainter painter(this);
    for (uint32_t i=1; i<numOfSegments; i++)
    {
        if (segments[i] == ADM_NO_PTS)
            continue;
        pos = (int)(((double)segments[i] * width()) / (double)totalDuration);
        if (pos < 1) pos = 1;
        if (pos > width() - 1) pos = width() - 1;
        if (pos == prevpos)
            painter.setPen(Qt::darkMagenta);
        else
            painter.setPen(Qt::red);
        prevpos = pos;

        if(layoutDirection() == Qt::LeftToRight)
            painter.drawLine(pos, 1, pos, height() - 3);
        else
            painter.drawLine(width() - pos, 1, width() - pos, height() - 3);
    }
}

void ADM_mwNavSlider::drawTimelineMarkers(void)
{
    if (timelineMarkers.empty() || !totalDuration || width() <= 1)
        return;

    QPainter painter(this);
    QColor markerColor = palette().color(QPalette::Link);
    markerColor.setAlpha(210);
    painter.setPen(QPen(markerColor, 1));
    painter.setBrush(markerColor);

    for (size_t i = 0; i < timelineMarkers.size(); ++i)
    {
        uint64_t time = timelineMarkers[i].timeUs;
        if (time > totalDuration)
            continue;
        int pos = static_cast<int>((static_cast<double>(time) * (width() - 1)) / static_cast<double>(totalDuration));
        pos = std::max(1, std::min(width() - 2, pos));
        if (layoutDirection() == Qt::RightToLeft)
            pos = width() - 1 - pos;
        painter.drawLine(pos, 1, pos, height() - 3);
        QPolygon flag;
        flag << QPoint(pos, 1) << QPoint(pos + 5, 1) << QPoint(pos, 6);
        painter.drawPolygon(flag);
    }
}

/**
    \fn paintEvent
*/
void ADM_mwNavSlider::paintEvent(QPaintEvent *event)
{
    drawCutPoints();

    QSlider::paintEvent(event);
    drawTimelineMarkers();
    drawSelection();
}
/**
    \fn setMarkerA
*/
void ADM_mwNavSlider::setMarkerA(uint64_t frameIndex)
{
    setMarkers(frameIndex, markerBTime);
}
/**
    \fn setMarkerB
*/
void ADM_mwNavSlider::setMarkerB(uint64_t frameIndex)
{
    setMarkers(markerATime, frameIndex);
}
/**
    \fn setMarkers
*/
void ADM_mwNavSlider::setMarkers(uint64_t frameIndexA, uint64_t frameIndexB)
{
    if (frameIndexA > totalDuration)
        printf("[ADM_QSlider] Marker A is out of bounds (%" PRIu64", %" PRIu64")\n", markerATime, totalDuration);
    else if (frameIndexB > totalDuration)
        printf("[ADM_QSlider] Marker B is out of bounds (%" PRIu64", %" PRIu64")\n", markerBTime, totalDuration);
    else
    {
        markerATime = frameIndexA;
        markerBTime = frameIndexB;

        repaint();
    }
}
/**
    \fn setTotalDuration
*/
void ADM_mwNavSlider::setTotalDuration(uint64_t duration)
{
    totalDuration = duration;
    repaint();
}
/**
    \fn setSegments
*/
void ADM_mwNavSlider::setSegments(uint32_t numOfSegs, uint64_t * segPts)
{
    if (segments)
        delete [] segments;
    segments = new uint64_t[numOfSegs];
    for (uint32_t i=0; i<numOfSegs; i++)
        segments[i] = segPts[i];
    numOfSegments = numOfSegs;
}
/*
    \fn wheelEvent
 */
void ADM_mwNavSlider::wheelEvent(QWheelEvent *e)
{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    //printf("Wheel : %d\n",e->delta());
    emit sliderAction(e->delta());
#else
    emit sliderAction(e->angleDelta().ry());
#endif
}

void ADM_mwNavSlider::setTimelineMarkers(const std::vector<ADM_TimelineMarker> &markers)
{
    timelineMarkers = markers;
    repaint();
}

void ADM_mwNavSlider::clearTimelineMarkers(void)
{
    timelineMarkers.clear();
    repaint();
}
//EOF
