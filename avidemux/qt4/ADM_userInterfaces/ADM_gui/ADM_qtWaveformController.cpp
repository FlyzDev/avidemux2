#include <atomic>
#include <memory>
#include <string>

#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <QTimer>

#include "ADM_default.h"
#include "ADM_files.h"
#include "ADM_edit.hxx"
#include "ADM_mwWaveform.h"
#include "ADM_qtWaveformController.h"
#include "ADM_waveformCache.h"
#include "ADM_waveformIndexer.h"
#include "ADM_waveformSnapshot.h"

namespace
{
static const uint32_t kDefaultBins = 8192;
static std::shared_ptr<std::atomic<bool> > cancelToken;
static std::string activeKey;
static bool startQueued = false;
static ADM_Composer *pendingComposer = NULL;
static QPointer<ADM_mwWaveform> pendingWaveform;

bool cancellationRequested(void *opaque)
{
    std::atomic<bool> *flag = static_cast<std::atomic<bool> *>(opaque);
    return flag && flag->load();
}

std::string cachePathForKey(const std::string &key)
{
    char *folder = ADM_getHomeRelativePath("waveform-cache");
    if (!folder)
        return std::string();

    std::string path;
    if (ADM_mkdir(folder))
    {
        path = folder;
        path += key;
        path += ".awf";
    }
    delete [] folder;
    return path;
}

class WaveformIndexTask : public QRunnable
{
public:
    WaveformIndexTask(const ADM_WaveformSnapshot &snapshot,
                      const std::string &key,
                      const std::string &cachePath,
                      const std::shared_ptr<std::atomic<bool> > &cancel,
                      const QPointer<ADM_mwWaveform> &target)
        : _snapshot(snapshot), _key(key), _cachePath(cachePath), _cancel(cancel), _target(target)
    {
        setAutoDelete(true);
    }

    void run(void) override
    {
        if (!_cancel || _cancel->load())
            return;

        std::shared_ptr<ADM_WaveformCacheData> data(new ADM_WaveformCacheData());
        bool cacheHit = false;
        bool ok = false;

        if (!_cachePath.empty())
        {
            cacheHit = ADM_readWaveformCache(_cachePath, _key, data.get());
            ok = cacheHit;
        }

        if (!ok && !_cancel->load())
        {
            std::string error;
            ok = ADM_generateWaveform(_snapshot, kDefaultBins, data.get(), &error,
                                      cancellationRequested, _cancel.get());
            if (!ok && !_cancel->load())
                ADM_warning("Waveform indexing failed: %s\n", error.c_str());
            if (ok && !_cancel->load() && !_cachePath.empty())
                ADM_writeWaveformCache(_cachePath, _key, *data);
        }

        if (!ok || _cancel->load())
            return;

        QCoreApplication *application = QCoreApplication::instance();
        if (!application)
            return;

        const std::string key = _key;
        const std::shared_ptr<std::atomic<bool> > cancel = _cancel;
        const QPointer<ADM_mwWaveform> target = _target;
        QMetaObject::invokeMethod(application, [key, cancel, data, cacheHit, target]() {
            if (!cancel || cancel->load() || key != activeKey || target.isNull())
                return;
            target->setDuration(data->durationUs);
            target->setChannelPeaks(data->tracks);
            ADM_info("Waveform ready (%s, %d track(s))\n",
                     cacheHit ? "cache" : "indexed", static_cast<int>(data->tracks.size()));
        }, Qt::QueuedConnection);
    }

private:
    ADM_WaveformSnapshot _snapshot;
    std::string _key;
    std::string _cachePath;
    std::shared_ptr<std::atomic<bool> > _cancel;
    QPointer<ADM_mwWaveform> _target;
};

void start(void)
{
    startQueued = false;

    if (cancelToken)
        cancelToken->store(true);
    cancelToken.reset();
    activeKey.clear();

    ADM_Composer *composer = pendingComposer;
    QPointer<ADM_mwWaveform> waveform = pendingWaveform;
    if (waveform.isNull())
        return;

    waveform->clearPeaks();
    if (!composer || !composer->isFileOpen())
        return;

    ADM_WaveformSnapshot snapshot;
    if (!ADM_buildWaveformSnapshot(composer, &snapshot) || snapshot.tracks.empty() || !snapshot.durationUs)
        return;

    const std::string key = ADM_waveformCacheKey(snapshot, kDefaultBins);
    if (key.empty())
        return;

    activeKey = key;
    cancelToken.reset(new std::atomic<bool>(false));
    QThreadPool::globalInstance()->start(
        new WaveformIndexTask(snapshot, key, cachePathForKey(key), cancelToken, waveform));
}
}

void ADM_QtWaveformController::schedule(ADM_Composer *composer, ADM_mwWaveform *waveform)
{
    pendingComposer = composer;
    pendingWaveform = waveform;
    if (startQueued || !waveform)
        return;

    startQueued = true;
    QTimer::singleShot(0, waveform, []() { start(); });
}

void ADM_QtWaveformController::cancel(void)
{
    if (cancelToken)
        cancelToken->store(true);
    cancelToken.reset();
    activeKey.clear();
    pendingComposer = NULL;
    pendingWaveform.clear();
    startQueued = false;
}
