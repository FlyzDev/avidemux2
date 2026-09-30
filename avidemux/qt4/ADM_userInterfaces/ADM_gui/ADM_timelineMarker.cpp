#include "ADM_timelineMarker.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QXmlStreamReader>

#include <algorithm>
#include <cmath>

namespace
{
QString normalizedKey(QString value)
{
    value = value.trimmed().toLower();
    value.remove(QRegularExpression("[^a-z0-9]"));
    return value;
}

uint64_t secondsToUs(double seconds)
{
    if (!std::isfinite(seconds) || seconds < 0.0)
        return 0;
    return static_cast<uint64_t>(std::llround(seconds * 1000000.0));
}

bool parseTimecode(const QString &text, double fps, uint64_t *timeUs)
{
    if (!timeUs)
        return false;
    QString value = text.trimmed();
    if (value.isEmpty())
        return false;

    bool numericOk = false;
    const double numeric = value.toDouble(&numericOk);
    if (numericOk)
    {
        *timeUs = secondsToUs(numeric);
        return true;
    }

    value.replace(';', ':');
    const QStringList parts = value.split(':');
    if (parts.size() < 2 || parts.size() > 4)
        return false;

    bool ok = true;
    double seconds = 0.0;
    if (parts.size() == 4)
    {
        bool a=false,b=false,c=false,d=false;
        const int hh=parts[0].toInt(&a), mm=parts[1].toInt(&b), ss=parts[2].toInt(&c), ff=parts[3].toInt(&d);
        ok = a && b && c && d && fps > 0.0;
        if (ok)
            seconds = hh * 3600.0 + mm * 60.0 + ss + ff / fps;
    }
    else if (parts.size() == 3)
    {
        bool a=false,b=false,c=false;
        const int hh=parts[0].toInt(&a), mm=parts[1].toInt(&b);
        const double ss=parts[2].toDouble(&c);
        ok = a && b && c;
        if (ok)
            seconds = hh * 3600.0 + mm * 60.0 + ss;
    }
    else
    {
        bool a=false,b=false;
        const int mm=parts[0].toInt(&a);
        const double ss=parts[1].toDouble(&b);
        ok = a && b;
        if (ok)
            seconds = mm * 60.0 + ss;
    }
    if (!ok || seconds < 0.0)
        return false;
    *timeUs = secondsToUs(seconds);
    return true;
}

void normalizeMarkers(std::vector<ADM_TimelineMarker> *markers)
{
    std::sort(markers->begin(), markers->end(), [](const ADM_TimelineMarker &a, const ADM_TimelineMarker &b) {
        if (a.timeUs != b.timeUs)
            return a.timeUs < b.timeUs;
        return a.name < b.name;
    });
    markers->erase(std::unique(markers->begin(), markers->end(), [](const ADM_TimelineMarker &a, const ADM_TimelineMarker &b) {
        return a.timeUs == b.timeUs && a.name == b.name;
    }), markers->end());
}

bool parseXml(const QByteArray &bytes, double fallbackFps,
              std::vector<ADM_TimelineMarker> *markers, QString *error)
{
    QXmlStreamReader xml(bytes);
    double fps = fallbackFps > 0.0 ? fallbackFps : 25.0;
    bool ntsc = false;
    int depth = 0;
    int sequenceDepth = -1;
    int sequenceRateDepth = -1;
    int clipDepth = -1;
    int markerDepth = -1;
    ADM_TimelineMarker current;
    QString markerIn;

    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.isStartElement())
        {
            ++depth;
            const auto name = xml.name();
            if (name == QLatin1String("sequence") && sequenceDepth < 0)
                sequenceDepth = depth;
            else if (sequenceDepth > 0 && name == QLatin1String("rate") &&
                     depth == sequenceDepth + 1 && sequenceRateDepth < 0)
                sequenceRateDepth = depth;
            else if (sequenceDepth > 0 && name == QLatin1String("clipitem") && clipDepth < 0)
                clipDepth = depth;
            else if (sequenceRateDepth > 0 && depth == sequenceRateDepth + 1 &&
                     name == QLatin1String("timebase"))
            {
                bool ok = false;
                const double parsed = xml.readElementText().trimmed().toDouble(&ok);
                if (ok && parsed > 0.0)
                    fps = parsed;
                --depth; // readElementText consumed the matching end element
            }
            else if (sequenceRateDepth > 0 && depth == sequenceRateDepth + 1 &&
                     name == QLatin1String("ntsc"))
            {
                ntsc = xml.readElementText().trimmed().compare(QLatin1String("TRUE"), Qt::CaseInsensitive) == 0;
                --depth;
            }
            else if (sequenceDepth > 0 && clipDepth < 0 && name == QLatin1String("marker"))
            {
                markerDepth = depth;
                current = ADM_TimelineMarker();
                markerIn.clear();
            }
            else if (markerDepth > 0 && depth == markerDepth + 1)
            {
                if (name == QLatin1String("name"))
                {
                    current.name = xml.readElementText().trimmed();
                    --depth;
                }
                else if (name == QLatin1String("comment"))
                {
                    current.comment = xml.readElementText().trimmed();
                    --depth;
                }
                else if (name == QLatin1String("in"))
                {
                    markerIn = xml.readElementText().trimmed();
                    --depth;
                }
            }
        }
        else if (xml.isEndElement())
        {
            const auto name = xml.name();
            if (markerDepth == depth && name == QLatin1String("marker"))
            {
                bool ok = false;
                const double frame = markerIn.toDouble(&ok);
                if (ok && frame >= 0.0 && fps > 0.0)
                {
                    double effectiveFps = fps;
                    if (ntsc && (std::fabs(fps - 30.0) < 0.1 || std::fabs(fps - 60.0) < 0.1))
                        effectiveFps = fps * 1000.0 / 1001.0;
                    current.timeUs = secondsToUs(frame / effectiveFps);
                    if (current.name.isEmpty())
                        current.name = QStringLiteral("Marker %1").arg(markers->size() + 1);
                    markers->push_back(current);
                }
                markerDepth = -1;
            }
            if (clipDepth == depth && name == QLatin1String("clipitem"))
                clipDepth = -1;
            if (sequenceRateDepth == depth && name == QLatin1String("rate"))
                sequenceRateDepth = -1;
            if (sequenceDepth == depth && name == QLatin1String("sequence"))
                sequenceDepth = -1;
            --depth;
        }
    }

    if (xml.hasError())
    {
        if (error)
            *error = QObject::tr("XML parse error: %1").arg(xml.errorString());
        return false;
    }
    if (markers->empty() && error)
        *error = QObject::tr("No sequence markers were found in the XML file.");
    return !markers->empty();
}

QStringList parseCsvRow(const QString &line)
{
    QStringList fields;
    QString field;
    bool quoted = false;
    for (int i=0; i<line.size(); ++i)
    {
        const QChar c=line.at(i);
        if (c == '"')
        {
            if (quoted && i + 1 < line.size() && line.at(i + 1) == '"')
            {
                field += '"';
                ++i;
            }
            else
                quoted = !quoted;
        }
        else if (c == ',' && !quoted)
        {
            fields << field.trimmed();
            field.clear();
        }
        else
            field += c;
    }
    fields << field.trimmed();
    return fields;
}

bool parseCsv(const QByteArray &bytes, double fallbackFps,
              std::vector<ADM_TimelineMarker> *markers, QString *error)
{
    QString text = QString::fromUtf8(bytes);
    QTextStream stream(&text, QIODevice::ReadOnly);
    QStringList headers;
    int nameCol=-1, commentCol=-1, timeCol=-1, frameCol=-1, secondsCol=-1, fpsCol=-1;
    bool firstRow = true;
    while (!stream.atEnd())
    {
        const QString line=stream.readLine();
        if (line.trimmed().isEmpty())
            continue;
        const QStringList fields=parseCsvRow(line);
        if (firstRow)
        {
            firstRow = false;
            headers=fields;
            bool recognizedHeader = false;
            for (int i=0;i<headers.size();++i)
            {
                const QString key=normalizedKey(headers[i]);
                if (key=="name" || key=="markername" || key=="title") { nameCol=i; recognizedHeader=true; }
                else if (key=="comment" || key=="description" || key=="notes") { commentCol=i; recognizedHeader=true; }
                else if (key=="time" || key=="timecode" || key=="start" || key=="in") { timeCol=i; recognizedHeader=true; }
                else if (key=="frame" || key=="frames") { frameCol=i; recognizedHeader=true; }
                else if (key=="seconds" || key=="second") { secondsCol=i; recognizedHeader=true; }
                else if (key=="fps" || key=="timebase") { fpsCol=i; recognizedHeader=true; }
            }
            if (recognizedHeader)
                continue;
            nameCol=0;
            timeCol=1; // simple headerless name,time CSV
        }

        auto get=[&](int col)->QString { return (col>=0 && col<fields.size()) ? fields[col] : QString(); };
        double fps=fallbackFps>0.0?fallbackFps:25.0;
        if (fpsCol>=0)
        {
            bool ok=false; const double f=get(fpsCol).toDouble(&ok); if(ok && f>0.0) fps=f;
        }
        uint64_t timeUs=0;
        bool have=false;
        if (frameCol>=0)
        {
            bool ok=false; const qint64 f=get(frameCol).toLongLong(&ok); if(ok && f>=0){timeUs=secondsToUs(f/fps); have=true;}
        }
        if (!have && secondsCol>=0)
        {
            bool ok=false; const double sec=get(secondsCol).toDouble(&ok); if(ok && sec>=0.0){timeUs=secondsToUs(sec); have=true;}
        }
        if (!have && timeCol>=0)
            have=parseTimecode(get(timeCol),fps,&timeUs);
        if (!have)
            continue;
        ADM_TimelineMarker marker;
        marker.timeUs=timeUs;
        marker.name=get(nameCol);
        marker.comment=get(commentCol);
        if(marker.name.isEmpty()) marker.name=QStringLiteral("Marker %1").arg(markers->size()+1);
        markers->push_back(marker);
    }
    if (markers->empty() && error)
        *error=QObject::tr("No marker rows could be read from the CSV file.");
    return !markers->empty();
}

bool markerFromJson(const QJsonObject &obj, double fallbackFps, ADM_TimelineMarker *marker)
{
    if (!marker)
        return false;
    marker->name = obj.value("name").toString(obj.value("title").toString(obj.value("label").toString()));
    marker->comment = obj.value("comment").toString(obj.value("description").toString());
    double fps = obj.value("fps").toDouble(obj.value("timebase").toDouble(fallbackFps > 0.0 ? fallbackFps : 25.0));

    if (obj.value("timeUs").isDouble() || obj.value("time_us").isDouble())
    {
        const double value = obj.value("timeUs").isDouble() ? obj.value("timeUs").toDouble() : obj.value("time_us").toDouble();
        if (value < 0.0) return false;
        marker->timeUs = static_cast<uint64_t>(std::llround(value));
        return true;
    }
    if (obj.value("timeMs").isDouble() || obj.value("time_ms").isDouble())
    {
        const double value = obj.value("timeMs").isDouble() ? obj.value("timeMs").toDouble() : obj.value("time_ms").toDouble();
        if (value < 0.0) return false;
        marker->timeUs = secondsToUs(value / 1000.0);
        return true;
    }
    if (obj.value("seconds").isDouble())
    {
        const double value = obj.value("seconds").toDouble();
        if (value < 0.0) return false;
        marker->timeUs = secondsToUs(value);
        return true;
    }

    const QJsonValue frameValue = obj.contains("frame") ? obj.value("frame") : obj.value("in");
    if (!frameValue.isUndefined())
    {
        bool ok = false;
        double frame = 0.0;
        if (frameValue.isDouble())
        {
            frame = frameValue.toDouble();
            ok = true;
        }
        else if (frameValue.isString())
            frame = frameValue.toString().toDouble(&ok);
        if (ok && frame >= 0.0 && fps > 0.0)
        {
            marker->timeUs = secondsToUs(frame / fps);
            return true;
        }
        if (frameValue.isString())
            return parseTimecode(frameValue.toString(), fps, &marker->timeUs);
    }

    const QJsonValue timeValue = obj.contains("time") ? obj.value("time") : obj.value("timecode");
    if (!timeValue.isUndefined())
    {
        if (timeValue.isDouble())
        {
            const double seconds = timeValue.toDouble();
            if (seconds < 0.0) return false;
            marker->timeUs = secondsToUs(seconds);
            return true;
        }
        if (timeValue.isString())
            return parseTimecode(timeValue.toString(), fps, &marker->timeUs);
    }
    return false;
}

bool parseJson(const QByteArray &bytes, double fallbackFps,
               std::vector<ADM_TimelineMarker> *markers, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        if (error) *error = QObject::tr("JSON parse error: %1").arg(parseError.errorString());
        return false;
    }

    QJsonArray array;
    double fps = fallbackFps > 0.0 ? fallbackFps : 25.0;
    if (doc.isArray())
        array = doc.array();
    else if (doc.isObject())
    {
        const QJsonObject root = doc.object();
        if (root.value("fps").isDouble()) fps = root.value("fps").toDouble();
        else if (root.value("timebase").isDouble()) fps = root.value("timebase").toDouble();
        array = root.value("markers").toArray();
    }

    for (const QJsonValue &value : array)
    {
        if (!value.isObject())
            continue;
        ADM_TimelineMarker marker;
        if (markerFromJson(value.toObject(), fps, &marker))
        {
            if (marker.name.isEmpty())
                marker.name = QStringLiteral("Marker %1").arg(markers->size() + 1);
            markers->push_back(marker);
        }
    }
    if (markers->empty() && error)
        *error = QObject::tr("No markers found in JSON.");
    return !markers->empty();
}
}

bool ADM_loadTimelineMarkers(const QString &fileName,
                             double fallbackFps,
                             std::vector<ADM_TimelineMarker> *markers,
                             QString *errorMessage)
{
    if (!markers)
        return false;
    markers->clear();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        if(errorMessage) *errorMessage=QObject::tr("Could not open %1").arg(fileName);
        return false;
    }
    const QByteArray bytes=file.readAll();
    const QString suffix=QFileInfo(fileName).suffix().toLower();
    bool ok=false;
    if(suffix=="xml") ok=parseXml(bytes,fallbackFps,markers,errorMessage);
    else if(suffix=="csv") ok=parseCsv(bytes,fallbackFps,markers,errorMessage);
    else if(suffix=="json") ok=parseJson(bytes,fallbackFps,markers,errorMessage);
    else
    {
        if(errorMessage) *errorMessage=QObject::tr("Unsupported marker file type: %1").arg(suffix);
        return false;
    }
    if(ok) normalizeMarkers(markers);
    return ok;
}
