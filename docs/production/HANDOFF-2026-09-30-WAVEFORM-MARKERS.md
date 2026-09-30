# Handoff — Avidemux Waveform + Timeline Markers

Date: 2026-09-30
Repo: `FlyzDev/avidemux2`
Active branch: `feat/waveform-ui`
Read first: `docs/production/STATE-OF-TRUTH.md`

## Start the next chat with this exact task

> In `FlyzDev/avidemux2`, read `docs/production/STATE-OF-TRUTH.md` and `docs/production/HANDOFF-2026-09-30-WAVEFORM-MARKERS.md`, sync `feat/waveform-ui`, and continue. FIRST solve the intermittent Previous/Next imported-marker seek error (`Error seeking to 75000 ms` in the user's Windows repro). Do not start release/macOS polish before this marker navigation bug is reproduced and fixed.

## Immediate P0: imported marker prev/next can throw a seek modal

The user has a real Windows reproduction. The marker file imports and renders, but using the imported-marker navigation buttons can sometimes show:

`Error seeking to 75000 ms`

This was reproduced while testing a ~3 minute video and the new marker buttons. The screenshot in the source chat clearly shows an Avidemux modal with this exact error.

### Current code path

`avidemux/qt4/ADM_userInterfaces/ADM_gui/Q_gui2.cpp`

```cpp
void MainWindow::seekTimelineMarker(bool forward)
{
    ...
    const uint64_t current = video_body->getCurrentFramePts();
    const uint64_t epsilon = 50000;
    ...
    if (target != ADM_NO_PTS)
        GUI_GoToTime(target);
}
```

At branch head `4e2f8fa`, this is around lines 329–364.

`GUI_GoToTime` is in `avidemux/common/gui_navigate.cpp` around lines 865–877:

```cpp
bool GUI_GoToTime(uint64_t time)
{
    if (false == video_body->goToTimeVideo(time))
    {
        GUI_Error_HIG(..., "Error seeking to ... ms", time / 1000);
    }
    admPreview::samePicture();
    GUI_setCurrentFrameAndTime();
    return true;
}
```

This is a naive exact seek. It displays the modal whenever `goToTimeVideo(time)` cannot directly decode that exact marker timestamp.

### Strong lead: reuse A/B marker navigation behavior

In the same `gui_navigate.cpp`, inspect the `ACT_GotoMarkA` / `ACT_GotoMarkB` case (roughly lines 174 onward at this commit). Avidemux already knows how to navigate to a marker robustly:

- save a rescue PTS;
- special-case beginning/end;
- try `goToTimeVideo(pts)`;
- on failure, find previous keyframe;
- if necessary try next keyframe;
- approach the desired marker from a decodable point;
- recover to the previous position on hard failure.

This is almost certainly the right behavior to share with imported markers. Prefer factoring a reusable robust `seek to timeline PTS` helper or otherwise reusing this path instead of inventing another seek strategy.

Do NOT simply suppress the error modal without fixing navigation.

### What to instrument first

Before changing behavior, log at least:

- current frame PTS;
- chosen imported marker target (`timeUs`);
- video duration;
- return value of direct `goToTimeVideo(target)`;
- previous keyframe candidate;
- next keyframe candidate;
- final actual current PTS after recovery.

Confirm whether 75,000 ms itself is valid timeline time but not an exact decodable frame PTS. The importer treats Premiere marker times as timeline times. For XMEML, `<in>` is converted from frame index using sequence `timebase` / `ntsc`.

### Acceptance test

Use a ~3-minute video and a five-marker XML. Navigate forward through all markers and backward through all markers repeatedly. No modal seek error. Landing on the nearest valid frame is acceptable and preferable to failure.

Also test:

- marker at/near 0;
- marker at/near end;
- marker between keyframes;
- CFR file;
- a file where exact PTS seeking previously failed;
- A/B marker navigation remains unchanged.

## Marker feature implementation

Commit: `2edf751` — `[nativewin][macarm] feat: import timeline markers`

Main files:

- `avidemux/qt4/ADM_userInterfaces/ADM_gui/ADM_timelineMarker.h`
- `avidemux/qt4/ADM_userInterfaces/ADM_gui/ADM_timelineMarker.cpp`
- `avidemux/qt4/ADM_userInterfaces/ADM_gui/ADM_mwNavSlider.h/.cpp`
- `avidemux/qt4/ADM_userInterfaces/ADM_gui/ADM_mwWaveform.h/.cpp`
- `avidemux/qt4/ADM_userInterfaces/ADM_gui/Q_gui2.h/.cpp`
- `avidemux/qt4/ADM_userInterfaces/ADM_gui/CMakeLists.txt`

UI:

- `M+` import markers
- `◀M` previous imported marker
- `M▶` next imported marker
- `Ctrl+Shift+M` import
- `Alt+Left` previous
- `Alt+Right` next

Imported markers are session-local and independent from A/B selection markers.

Formats:

- Premiere / XMEML XML: parses sequence `timebase`, `ntsc`, marker `name`, `comment`, `in`.
- CSV: common header variants plus simple `name,time`.
- JSON: array or `{ "markers": [...] }` with multiple time representations.

Marker parser tests were run locally for XML / CSV / JSON and passed. Widget/parser syntax checks passed. Windows cloud build passed.

## Waveform feature implementation / important history

The waveform work is not a superficial paint-only patch. It contains:

- peak accumulator;
- cache format / fingerprinting;
- timeline snapshot;
- independent internal/external audio indexing;
- Qt waveform widget;
- async controller;
- multi-track/channel views;
- progressive updates;
- cancellation;
- background work throttling;
- Windows Unicode path handling;
- Windows native package + portable postprocess CI.

Key files:

- `avidemux/common/ADM_editor/include/ADM_waveformPeak.h`
- `avidemux/common/ADM_editor/src/audio/ADM_waveformPeak.cpp`
- `avidemux/common/ADM_editor/include/ADM_waveformCache.h`
- `avidemux/common/ADM_editor/src/audio/ADM_waveformCache.cpp`
- `avidemux/common/ADM_editor/include/ADM_waveformSnapshot.h`
- `avidemux/common/ADM_editor/src/audio/ADM_waveformSnapshot.cpp`
- `avidemux/common/ADM_editor/include/ADM_waveformIndexer.h`
- `avidemux/common/ADM_editor/src/audio/ADM_waveformIndexer.cpp`
- `avidemux/qt4/ADM_userInterfaces/ADM_gui/ADM_mwWaveform.h/.cpp`
- `avidemux/qt4/ADM_userInterfaces/ADM_gui/ADM_qtWaveformController.h/.cpp`

### Critical Windows hang already solved

The first async implementation could still hang the GUI on real media, especially while background indexing reopened the source. A live Windows hang was captured:

- Avidemux was genuinely `Responding=False`.
- Full dump was taken on the user's Mainframe:
  - `C:\Users\musta\Documents\AvidemuxHangDumps\avidemux_hang_live_63532.dmp`
  - about 809 MB at capture time.
- Windows wait-chain / window inspection showed the main Avidemux UI involved in synchronous cross-thread GUI interaction with a worker-created progress window.
- Source inspection showed demux open can create `Decoding frame type` / processing UI.

Fix: `e18433c` — `[nativewin] fix: keep demuxer progress UI off waveform worker`

The fix uses the existing thread-local `GUI_SuppressProgressForCurrentThread(bool)` mechanism so the waveform worker cannot open progress UI while independent demuxing/indexing. Do not regress this by switching to global `GUI_Quiet()` or creating Qt widgets from the worker.

## Windows build / exact artifacts

Marker code full native build:

- run `36649741097`
- success
- head `2edf751`

Marker portable postprocess:

- run `36679279732`
- success
- validation head `4e2f8fa`
- artifact ID `11081366719`
- artifact name `avidemux-waveform-windows-portable-x64`
- GitHub artifact digest `sha256:ee40fe11b1f19cf3a8a2b41c364b9ab3731bf824594577db5e8fceb342073d6c`

User Mainframe currently has the official CI inner portable zip copied to:

- `C:\Users\musta\Downloads\Avidemux-Waveform-Markers-Windows-x64.zip`
- `C:\Users\musta\Downloads\avidemux-waveform-win64-portable.zip`

Inner zip SHA-256:

- `A7CC5800E6874D54D4E421CC50496DE1B03431074CB5DAA3D4A94471CA45BA28`

Mainframe Remote Desktop Commander device:

- `cf5c57b1-3c3a-44ee-81a6-4f6393fdb4be`

The Mainframe is currently online as of this handoff, but do not assume it remains online in the next chat.

## macOS Apple Silicon work

Goal: publish a self-contained Apple Silicon `.app` and DMG from the same fork.

Workflow:

- `.github/workflows/macos-arm64-waveform-build.yml`
- self-hosted runner: `avidemux-waveform-mac-arm64`
- Mac mini device: `e839f56f-5a89-4f77-a40e-abaa5ac162f4`
- workspace: `/Users/flyzai/agent-workspaces/avidemux-waveform-work`

Case-sensitive build requirement:

- workflow creates/uses `/Volumes/AvidemuxWaveformCI` on a case-sensitive APFS sparse image;
- submodules must be initialized, especially `avidemux/qt4/i18n`.

Landing-page/mac workflow commit:

- `1d69505` — `[macarm] feat: publish Apple Silicon build and project landing page`

Marker-enabled mac run:

- run `36649741021`
- failed after approximately 51 minutes
- marker code compiled far enough to reach app/DMG packaging
- no artifact uploaded because failure occurred before upload step
- strict `codesign --verify --deep --strict` rejected the ad-hoc app bundle with a resource/signature mismatch (`code has no resources but signature indicates they must be present`).

After P0 marker seek is fixed:

1. inspect upstream Avidemux macOS signing/packaging expectations;
2. adjust verification to match the ad-hoc bundle model instead of blindly enforcing an incompatible strict resource-envelope check;
3. still verify architecture (`arm64`), DMG integrity (`hdiutil verify`) and smoke-launch the packaged `Avidemux2.8` binary with the multi-track fixture;
4. upload DMG + app zip + SHA256 file.

## GitHub page / release status

Repository metadata was updated globally:

- description: waveform-focused Avidemux fork;
- homepage: `releases/latest`;
- topics: Avidemux, video editing, waveform, audio waveform, Qt6, FFmpeg, Windows, macOS.

README was rewritten on the feature branch to include:

- project purpose;
- download table;
- Windows/macOS install notes;
- waveform features;
- marker import docs;
- build notes;
- upstream/license attribution.

Important: GitHub default branch remains `master`. The redesigned README currently lives on `feat/waveform-ui`, so it will not become the public default landing page until the stable feature work is merged to `master` (or default branch is intentionally changed).

There is no final public preview release containing both Windows and macOS packages yet.

Recommended release sequence AFTER P0 + macOS pass:

1. Windows full native + portable postprocess success on final commit.
2. macOS ARM64 packaged app/DMG smoke success on same logical feature set.
3. Merge `feat/waveform-ui` to `master` (preserve upstream relationship).
4. Tag a preview release (e.g. a sensible `waveform-preview-*` tag).
5. Attach Windows portable zip, macOS DMG, optional app zip, and SHA256 checksums.
6. Release notes must explicitly call this an unofficial community fork, summarize waveform + marker features, and link upstream Avidemux.

## Git / coordination rules for next chat

Before editing:

```bash
cd /Users/flyzai/agent-workspaces/avidemux-waveform-work
git fetch origin feat/waveform-ui
git reset --hard origin/feat/waveform-ui
```

Then read:

- `docs/production/STATE-OF-TRUTH.md`
- this handoff

Do not overwrite incoming parallel-lane commits. Inspect `git log` before each patch.

When Windows code changes are ready, use the existing workflow triggers intentionally:

- commit containing `[nativewin]` triggers full native Windows package;
- `[winpost]` triggers portable postprocess using the latest successful native package;
- `[macarm]` triggers Apple Silicon packaging.

Do not claim a downloadable build exists until the artifact actually exists and passes the relevant smoke step.
