#pragma once

#include <QString>
#include <stdint.h>
#include <vector>

struct ADM_TimelineMarker
{
    uint64_t timeUs = 0;
    QString name;
    QString comment;
};

bool ADM_loadTimelineMarkers(const QString &fileName,
                             double fallbackFps,
                             std::vector<ADM_TimelineMarker> *markers,
                             QString *errorMessage);
