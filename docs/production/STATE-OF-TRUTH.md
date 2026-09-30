# Avidemux Waveform — State of Truth

Last updated: 2026-09-30

This file is the compact source of truth for the `FlyzDev/avidemux2` waveform fork. Detailed continuation notes live in `docs/production/HANDOFF-2026-09-30-WAVEFORM-MARKERS.md`.

## Repository

- GitHub: `FlyzDev/avidemux2`
- Upstream: `mean00/avidemux2`
- Active branch: `feat/waveform-ui`
- Current code commit containing marker import: `2edf751` (`[nativewin][macarm] feat: import timeline markers`)
- Current branch head after portable validation: `4e2f8fa` (`[winpost] validate marker-enabled portable build`)
- Local Mac mini workspace: `/Users/flyzai/agent-workspaces/avidemux-waveform-work`
- Default GitHub branch is still `master`; the feature branch has NOT yet been merged to `master`.

## Stable / working

### Waveform UI

- Combined/master waveform is the default view.
- Right-click can show separate audio tracks or separate channels.
- Waveform sits inside the navigation layout and no longer covers transport/time controls.
- Left-click waveform seeking is implemented.
- Playhead and A/B selection markers are drawn over the waveform.
- Waveform peak cache is implemented and reused when the source/timeline fingerprint matches.

### Background waveform generation

- Indexing runs on a dedicated low-priority `QThread` instead of the shared UI/global thread pool.
- Waveform generation starts after a short delay so the opened video becomes usable first.
- Progressive peak data is delivered while indexing continues.
- Background decode yields periodically to foreground playback/seek work.
- Critical Windows hang was diagnosed with a live ProcDump / Wait Chain capture: the waveform worker could reopen a demuxer which created progress UI (`Decoding frame type`) from the worker thread, producing a cross-thread Qt/Windows GUI lock.
- Fix commit: `e18433c` (`[nativewin] fix: keep demuxer progress UI off waveform worker`). It uses per-thread progress-UI suppression around the independent waveform demux/index work, without globally silencing the normal UI.

### Timeline marker import

Implemented in commit `2edf751`.

- Formats: Premiere / XMEML XML, CSV, JSON.
- Premiere/XMEML sequence `timebase` and `ntsc` are parsed; marker `<in>` values are interpreted as frames.
- CSV accepts common marker/name/time/frame/fps columns and simple `name,time` files.
- JSON accepts an array or `{ "markers": [...] }`, with `timeUs`, `timeMs`, `seconds`, `frame`, `in`, `time`, or `timecode` time fields.
- Imported markers are separate from Avidemux A/B markers.
- Imported markers are drawn on both the main seek slider and the waveform.
- Marker names can be drawn on the waveform.
- Controls:
  - `M+` — import marker file
  - `◀M` — previous imported marker
  - `M▶` — next imported marker
  - `Ctrl+Shift+M` — import
  - `Alt+Left` / `Alt+Right` — previous / next marker
- Parser tests for XML / CSV / JSON passed locally.
- Windows native build with marker support passed.

## P0 — open blocker: imported marker navigation seek error

**This is the FIRST issue to solve in the next chat. Do not start release work before it is fixed and reproduced cleanly.**

User reproduction on Windows:

- Import markers into a ~3 minute video.
- Press previous / next imported-marker button.
- Some marker targets work, but some can show a modal error such as:
  - `Error seeking to 75000 ms`
- Screenshot was captured in the originating chat on 2026-09-30.

Current marker navigation implementation:

- `avidemux/qt4/ADM_userInterfaces/ADM_gui/Q_gui2.cpp`
- `MainWindow::seekTimelineMarker(bool forward)` around lines 329–364 at commit `4e2f8fa`.
- It chooses an imported marker timestamp and calls `GUI_GoToTime(target)` directly.

Error source:

- `avidemux/common/gui_navigate.cpp`
- `GUI_GoToTime(uint64_t time)` around lines 865–877.
- It calls `video_body->goToTimeVideo(time)` once and opens `GUI_Error_HIG(...)` when the requested microsecond timestamp is not directly seekable / decodable.

Very important comparison already identified:

- Existing Avidemux **A/B marker navigation** in `gui_navigate.cpp` around the `ACT_GotoMarkA` / `ACT_GotoMarkB` case already handles this exact class of failure much more robustly.
- When `goToTimeVideo(pts)` fails, it tries previous keyframe / next keyframe and approaches the requested marker, preserving a rescue PTS.
- Imported marker navigation should likely reuse or factor out this robust logic instead of calling naive `GUI_GoToTime(target)`.
- Do not assume the imported timestamp itself is malformed just because `goToTimeVideo` rejects it: Premiere marker time is a timeline time, while Avidemux may require a decodable frame PTS / keyframe-assisted seek.

Acceptance criteria for P0:

1. Import the 5-marker test XML into a ~3 minute video.
2. Repeatedly press previous / next through every marker in both directions.
3. No `Error seeking to ... ms` modal.
4. Navigation lands on the marker or the nearest valid frame consistently.
5. A/B marker navigation remains unchanged.
6. No regression in normal timeline dragging or waveform generation.

## Windows build status

Marker-enabled full native package:

- Workflow: `Windows native waveform package`
- Run: `36649741097`
- Result: success
- Head: `2edf751`

Marker-enabled portable validation:

- Workflow: `Windows waveform portable postprocess`
- Run: `36679279732`
- Result: success
- Head: `4e2f8fa`
- Artifact ID: `11081366719`
- Artifact name: `avidemux-waveform-windows-portable-x64`
- GitHub artifact digest: `sha256:ee40fe11b1f19cf3a8a2b41c364b9ab3731bf824594577db5e8fceb342073d6c`

Mainframe test PC currently has the official CI portable package in:

- `C:\Users\musta\Downloads\Avidemux-Waveform-Markers-Windows-x64.zip`
- Same package is also copied to `C:\Users\musta\Downloads\avidemux-waveform-win64-portable.zip`
- Inner portable ZIP SHA-256: `A7CC5800E6874D54D4E421CC50496DE1B03431074CB5DAA3D4A94471CA45BA28`

Remote Desktop Commander Mainframe device ID:

- `cf5c57b1-3c3a-44ee-81a6-4f6393fdb4be`

## macOS Apple Silicon status

The Apple Silicon pipeline is close but not publishable yet.

- Workflow: `macOS ARM64 waveform build`
- Marker-enabled run: `36649741021`
- Result: failure after a long successful build/package path; no artifact uploaded.
- The workflow reached creation of the self-contained `.app` / DMG but the strict verification step failed with an ad-hoc bundle signature/resource mismatch (`code has no resources but signature indicates they must be present`).
- Workflow file: `.github/workflows/macos-arm64-waveform-build.yml`
- The workflow uses a case-sensitive APFS sparse image because Avidemux requires a case-sensitive source/build filesystem on macOS.
- It is intended to publish:
  - `Avidemux-Waveform-macOS-Apple-Silicon.dmg`
  - `Avidemux-Waveform-macOS-Apple-Silicon.app.zip`
- After P0 marker seek is fixed, repair the codesign verification to match the upstream/ad-hoc packaging model, then smoke launch the packaged app with the multi-track fixture before publishing.

Mac mini runner / workspace:

- Device: `e839f56f-5a89-4f77-a40e-abaa5ac162f4`
- Workspace: `/Users/flyzai/agent-workspaces/avidemux-waveform-work`
- Self-hosted runner name: `avidemux-waveform-mac-arm64`

## GitHub / public release status

Already done:

- Repository description updated to describe the waveform fork.
- Homepage points to `releases/latest`.
- Topics include Avidemux, waveform, video editing, Qt6, FFmpeg, Windows and macOS.
- Feature-branch README has been redesigned as a product landing page with download/install/feature/marker sections.

Not done yet:

- No public stable/preview GitHub Release containing both Windows and macOS packages.
- Feature branch is not merged to default `master`, so the redesigned README is not yet the default GitHub landing page.
- Do not merge/publish the release until P0 marker navigation is fixed and the macOS artifact passes smoke verification.

## Next order of work

1. **P0: Fix previous/next imported-marker navigation seek failures.**
2. Add a regression path for marker navigation (at minimum a deterministic manual/integration fixture; preferably a reusable helper test).
3. Rebuild Windows native + portable and verify the user's failing 3-minute case.
4. Fix macOS packaging verification and produce smoke-tested ARM64 DMG/app zip.
5. Merge the stable feature branch to `master` (or otherwise make the redesigned README the default landing page).
6. Create a GitHub preview release and upload Windows + macOS packages with checksums and concise release notes.
