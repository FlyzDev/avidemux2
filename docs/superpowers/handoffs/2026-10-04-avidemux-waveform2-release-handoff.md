# Avidemux Waveform Fork — v2.8.2-waveform.2 Final Handoff

Date: 2026-10-04
Repository: `FlyzDev/avidemux2`
Primary workspace: `/Users/flyzai/agent-workspaces/avidemux-waveform-work`
Default branch: `master`
Upstream: `mean00/avidemux2`

## Executive status

The waveform/marker fork has been finalized as `v2.8.2-waveform.2` and published as a normal GitHub Release.
The release tag points exactly to commit `99f7ff7ce88514cc8b825804adc32f8307feff3b`.
At release time, local `master`, `origin/master`, and the release tag source were aligned and the source worktree was clean.

GitHub release:
`https://github.com/FlyzDev/avidemux2/releases/tag/v2.8.2-waveform.2`

Final release title:
`Avidemux Waveform + Markers 2.8.2 — waveform.2`

This handoff is intentionally detailed. A future chat should read this file first before changing Avidemux.
## Final release artifacts and checksums

Published assets were downloaded back from the GitHub Release after publishing and verified against the published `SHA256SUMS.txt`.

- `Avidemux-Waveform-Windows-x64-Portable.zip`
  - SHA256: `22f47a19def8c59888cbebf683fc91bfec168bb11ed4d8950c822791b2a16050`
- `Avidemux-Waveform-macOS-Apple-Silicon.dmg`
  - SHA256: `e7173912be0072cb90ee927ce0710075ab2e412ecea55b2e8747077e446c5599`
- `Avidemux-Waveform-macOS-Apple-Silicon.app.zip`
  - SHA256: `b1dc9e16e917844907d2154f34c2c9a66b89af7d6d1c7c6455c4dedf3705a49b`
- `SHA256SUMS.txt`

Fresh post-publish re-download verification result:
- Windows ZIP: OK
- macOS DMG: OK
- macOS app ZIP: OK

Do not replace these assets without creating a new release/tag unless the user explicitly asks to rewrite release history.
## CI evidence for release SHA `99f7ff7ce`

All release-critical CI jobs for the final SHA completed successfully:

- macOS ARM64 waveform build: run `37219723161` — SUCCESS
  - checkout: success
  - case-sensitive source worktree: success
  - self-contained ARM64 app + DMG build: success
  - multi-track fixture generation: success
  - LaunchServices smoke launch from packaged DMG: success
  - artifact staging/upload/cleanup: success
- Windows native waveform package: run `37219723174` — SUCCESS
- Windows waveform build: run `37219723180` — SUCCESS
- Final Windows portable postprocess: run `37221683525` — SUCCESS
  - native package discovery/download: success
  - MSYS2 runtime source: success
  - PE dependency resolution: success
  - multi-track smoke launch: success
  - portable package/upload: success

The automatic postprocess runs created by normal pushes can be `skipped`; the final portable run above was deliberately workflow-dispatched after the final native package succeeded.
## Fresh source verification at finalization

All repository regression scripts were re-run on final `master` before release publication:

- `tests/test_macos_unique_dmg_volume.py` — PASS
- `tests/test_multimonitor_screen_geometry.py` — PASS
- `tests/test_opengl_runtime_fallback.py` — PASS
- `tests/test_playback_preserves_window_geometry.py` — PASS
- `tests/test_silent_save_notification.py` — PASS
- `tests/test_slider_release_fine_seek.py` — PASS
- `tests/test_waveform_window_growth.py` — PASS
- `tests/test_windows_portable_packaging.py` — PASS
- `git diff --check` — PASS

Important verification discipline: source tests and CI are not substitutes for runtime behavior when the reported bug is visual, audible, DPI/monitor-specific or driver-specific. Reproduce the user's exact symptom when changing those areas.
## macOS release verification details

The final macOS CI artifact was also downloaded and independently inspected on the Mac mini.

Verified locally:
- DMG checksum matches CI checksum file.
- `hdiutil verify` reports the DMG checksum as VALID.
- app ZIP passes `unzip -t` with no errors.
- extracted `Avidemux-2.8.2.app` passes `codesign --verify --deep --strict` and satisfies its designated requirement.
- bundle executable from `CFBundleExecutable` is `Avidemux2.8`.
- executable is `Mach-O 64-bit executable arm64`.
- `LC_BUILD_VERSION minos` is `14.0`.

One verification command initially looked for `Contents/MacOS/avidemux`, which does not exist in the bundle. That was a verification-command mistake, not a build failure. The executable name was then read from `Info.plist` and the ARM64/minimum-OS verification passed.

The release is ad-hoc signed, not Apple-notarized. README tells users they may need Right click → Open on first launch.
## Windows deployment on Mainframe

Mainframe device ID: `cf5c57b1-3c3a-44ee-81a6-4f6393fdb4be`.

The exact `.2` release Windows ZIP was downloaded from the published GitHub Release and its SHA256 was verified before deployment.
It was extracted to a temporary test folder and smoke-launched with the packaged multi-track fixture for 8 seconds. Result: `RELEASE_STAGE_SMOKE_PASS`.

Final deployed location:
`C:\Users\musta\Downloads\Avidemux-Waveform-Final\avidemux_portable.exe`

Latest release ZIP copy:
`C:\Users\musta\Downloads\Avidemux-Waveform-Windows-x64-Portable-Latest.zip`

The existing portable `settings` directory was preserved across the swap. The exact release copy was then smoke-launched again. Result: `FINAL_RELEASE_SMOKE_PASS`.
The final ZIP hash on Mainframe is `22F47A19DEF8C59888CBEBF683FC91BFEC168BB11ED4D8950C822791B2A16050`.
Temporary deployment/backup directories were removed after the final smoke test passed.
## Core feature set already in this fork

The fork's purpose is fast copy-mode editing with waveform and timeline-marker ergonomics, not a redesign of Avidemux.

Implemented before `.2` and retained:
- progressive cached audio waveform generation
- combined/master, separate-track and separate-channel waveform views
- multi-track background waveform indexing
- waveform playhead and A/B overlays
- Premiere/XMEML XML, CSV and JSON timeline marker import
- imported marker lines and labels on timeline/waveform
- previous/next imported marker controls and navigation
- marker navigation tolerant of timestamps which do not exactly match a decoded frame PTS
- Windows x64 portable packaging
- macOS Apple Silicon app/DMG packaging

User preference: preserve normal A/B semantics and normal copy-mode behavior. Fork-specific marker fixes should not silently alter stock A/B behavior.
## Important commits leading to `.2`

Key commits, newest-to-older context:
- `99f7ff7ce` — `[nativewin][macarm] ci: avoid reused macOS DMG volume`
  - final release SHA
  - macOS DMG packaging uses a unique internal volume name and cleans stale mounts to avoid LaunchServices/App Management reuse failures
- `f251abc04` — `[nativewin][macarm] release: prepare waveform.2`
  - added/updated `PATCH_NOTES.md` and README release documentation
- `c469afb7d` — `[nativewin] fix mutable waveform resize callback`
  - corrected compile failure caused by `const auto` on a `mutable` lambda
  - strengthened regression coverage so this exact source mistake is caught
- `955589936` — `[qt] grow window with expanded waveform`
  - when waveform view grows from Combined to Tracks/Channels, grow outer window by waveform-height delta when possible instead of covering video
- `acf88896e` — `[nativewin] fall back when OpenGL runtime extension is missing`
  - prevents Deniz's OpenGL crash when runtime `glActiveTexture` resolution fails
- `8fb399583` — `[nativewin][macarm] fix: hold resize guard through pause restore`
  - final Play/Pause geometry-preservation implementation
Additional important commits:
- `d2f3d0b41` — first timing-based playback geometry attempt; **runtime-failed on Pause** and was superseded by `8fb399583`
- `47903ee5e` — prefer `windowHandle()->screen()` over `screenAt(frameGeometry().center())` on mixed-DPI Windows
- `0318f323b` — first current-monitor resize fix; introduced current-screen geometry helper
- `75deb7a76` — successful saves use a non-modal status message to avoid the unwanted Windows notification sound
- `f76d433c` — timeline mouse release performs fine/exact seek; drag remains fast/keyframe-oriented
- `442607f5` — ffnvcodec CMake detection fix
- `a6a1d28d` — NVDEC/NVENC workflow dependency support
- `91926f535` — portable packaging fix; portable executable is `avidemux_portable.exe`
- `09ad53ebd` — portable smoke selector fix
- `6eab8243` — waveform visibility/scaling fix
- `2bd0229` — nearest-frame imported-marker navigation fix
- `2edf751` — marker import foundation
- `c987bb505` — macOS package fix used as `.1` release baseline

When investigating regressions, use `git show <sha>` rather than re-implementing from memory.
## Multi-monitor and Play/Pause geometry history

Mainframe has a mixed-DPI three-monitor layout. Historical geometry seen during debugging:
- left secondary: `DISPLAY2`, 2560×1440, positioned left of primary
- primary: `DISPLAY3`, logical bounds around 3072×1728
- right secondary: `DISPLAY1`, 3072×1728, positioned to the right

Root cause of the original monitor jump:
- `Q_gui2.cpp::UI_resize()` used primary-screen available geometry
- secondary-window coordinates were therefore judged against primary bounds
- first fix selected the current screen, but `screenAt(frameGeometry().center())` could still be wrong under mixed-DPI coordinate spaces
- final screen selection prefers `window->windowHandle()->screen()` and falls back only when needed

Root cause of Play/Pause size changes:
- playback dimension changes call `admPreview::setMainDimension()`
- `T_preview.cpp::UI_updateDrawWindowSize()` can call top-level `UI_resize()`
- the first timing-based guard ended too early; Pause restore still resized the window
- `8fb399583` introduced an explicit resizing block flag held through the full playback-stop/restore lifecycle.
Historical runtime evidence for the failed timing guard (`d2f3d0b4`):
- right-monitor baseline and first Play stayed stable
- first Pause changed the window rectangle
- therefore the source-level timing guard was explicitly rejected and not deployed

After `8fb399583`, actual Win32/UIAutomation tests were run on both right and left secondary monitors. The sequence was Play → Pause → Play → Pause, and all observed window rects remained equal to the measured baseline while the timeline slider advanced during playback. This is the runtime evidence supporting the geometry fix.

Do not regress this by reintroducing renderer-driven top-level resize during playback. If touching these paths, rerun full RECT equality tests on both secondary monitors, not just source regressions.
## Waveform Tracks/Channels UI growth bug

User screenshots showed a small UI problem: switching waveform from Combined to Tracks or Channels increased the navigation/waveform region but the outer app window did not grow. The new area therefore covered/consumed the video preview. Choosing a View → Zoom action made it look correct because that triggered another outer-window size calculation.

Fix in `955589936`:
- track the previous preferred waveform height
- calculate `delta = waveformHeight - previousWaveformHeight`
- resize the top-level window by the applied delta when there is room
- use the current monitor work area as the clamp
- do not force-resize maximized or fullscreen windows
- block zoom changes during this outer resize

The first implementation compiled incorrectly because a stateful `mutable` lambda was declared `const auto`. CI caught it. `c469afb7d` changed this to `auto` and added a regression assertion which rejects `const auto resizeNavigationForWaveform`.

Status at release: source regression PASS and Windows package smoke PASS. If the user reports that the visual behavior is still off on their exact layout, reproduce visually rather than assuming the regression test proves UX correctness.
## Deniz OpenGL crash and fallback

Crash stack received from Deniz included:
`ADM_coreQtGl::uploadAllPlanes(ADMImage*)` → `QtGlAccelWidget::setImage()` → render update during video open.

The key failure was a required OpenGL function pointer (`glActiveTexture`) being unavailable on Deniz's OpenGL/driver/context combination. The application was still allowed to instantiate the OpenGL renderer and later asserted on first-frame upload.

Mainframe could not reproduce the crash even with OpenGL enabled and the same video, so the media file itself was not treated as the root cause.

Fix in `acf88896e`:
- OpenGL renderer factory/runtime capability is checked before committing to that renderer
- if required extension resolution fails, fall back to the normal renderer instead of proceeding to a null function pointer/assert

Temporary workaround before the fix was to disable OpenGL display and use DXVA2/normal rendering. Keep the fallback behavior even if a future machine cannot reproduce Deniz's exact driver state.
## Save-completion sound behavior

User reported that Avidemux produced an unwanted notification sound after successful saves.
Two attempts to keep a modal success popup while making it silent still produced sound on Windows.

Final behavior from `75deb7a76`:
- successful save: no modal QMessageBox; show a status notification for 4 seconds
- failed save: keep the error popup

Current success path conceptually calls `UI_notifyInfo(message, 4000)` after `A_Save(name)` succeeds.
There is no user-facing on/off toggle for the old completion sound.

If the user later asks why the success popup disappeared, this was intentional: modal variants continued to trigger the sound. Do not restore a modal success popup without proving it stays silent on Windows.
## Timeline, A/B and marker behavior

A/B itself was investigated and stock A/B jumping was not found broken. Do not describe A/B as a fork bug without new evidence.

Stock Avidemux timeline clicking normally performs keyframe-oriented seeking. The fork's final behavior is deliberate:
- dragging the timeline stays fast/keyframe-oriented
- mouse release performs a fine/exact seek for accurate positioning
- imported marker navigation uses nearest-frame behavior so non-exact timestamps do not produce seek errors
- fork-specific marker behavior should not alter A/B semantics

Historical 29.97 fps marker validation used deterministic marker positions and confirmed navigation in both directions without seek-error modal. If modifying marker navigation, keep a 29.97 fps regression because exact PTS equality was the original failure mode.
## NVDEC / NVENC status

The earlier Windows hardware-codec investigation found the main problem was build-time ffnvcodec detection, not an inherently obsolete Avidemux codebase.

Final compile-time proof from the corrected workflow included:
- ffnvcodec found (historically observed version `13.0.19.0`)
- NVENC headers found
- `NVENC Yes`
- `USE_NVENC` defined
- Avidemux ffmpeg configuration includes NVENC/NVDEC when `USE_NVENC` is enabled

Important wording: this is compile-time support evidence. It is not proof that a real playback session used NVDEC for decoding. Do not claim runtime NVDEC acceleration was verified unless a future test explicitly measures/observes it.
## Portable Windows packaging rules

The portable package must use `avidemux_portable.exe`, not a normally named `avidemux.exe`. This is what activates local portable settings behavior.

Expected portable proof in logs:
- `[isPortableMode] Portable mode`
- settings/jobs/log base points to the `settings` directory beside the executable

The portable workflow resolves PE dependencies, smoke-launches a generated multi-track fixture, verifies the local settings directory is created, then packages the runtime.

Important workflow detail: `.github/workflows/windows-postprocess.yml` currently has `workflow_dispatch:` with **no custom inputs**. A prior attempt to call it with `-f source_run_id=...` failed with HTTP 422. Correct final usage is simply:
`gh workflow run windows-postprocess.yml --repo FlyzDev/avidemux2`

The workflow then finds the latest successful native Windows package on the current branch.
## macOS finalization failure that blocked release, and the fix

The first `.2` finalization attempt was deliberately not published because the macOS ARM64 job did not complete cleanly on the release-prep SHA.
The failure was in packaging/smoke lifecycle, not in the feature source itself.

The issue involved reuse of a DMG volume/mount identity after previous smoke launches, interacting badly with macOS LaunchServices/App Management state. The final workflow change in `99f7ff7ce`:
- cleans stale mounted Avidemux test DMGs before packaging
- gives the DMG a unique internal volume name per CI run
- retains strict DMG/signing/LaunchServices verification

After this change, macOS run `37219723161` passed every step and produced the final release artifacts.

Do not remove the unique-volume/stale-mount handling merely because a one-off local DMG build works. It exists to make repeated CI packaging reliable.
## Git / branch state at finalization

There was no remaining waveform feature branch to merge: the active finalized implementation was already on `master`, and `origin/master` matched it. Remote branches visible at finalization were only `origin/master` plus `upstream/master`.

Release code/tag state:
- release source commit: `99f7ff7ce88514cc8b825804adc32f8307feff3b`
- tag: `v2.8.2-waveform.2`
- tag target verified after fetching the remote tag

There is one separate local worktree/branch:
- worktree: `/Users/flyzai/agent-workspaces/avidemux-win-native-exp`
- branch: `win-native-experiment`
- HEAD: `eb6284e3d6780e20148985e0c5cf8f4d2c07c4c2`

That branch was **not** listed as merged into current `master`. It is an old native-Windows experiment and was intentionally not force-deleted during release cleanup. Review it before deleting; do not discard unmerged commits automatically.
## Important runtime fixture and user validation

Main Windows test video used repeatedly:
`C:\Users\musta\Documents\Deniz Bağlantı\Escape the Backrooms 2026.09.28 - 23.48.18.mkv`

Historical properties: roughly 179.867 seconds, six stereo AAC tracks. Some tracks are nearly silent; others contain mic/game/combined content. It is useful for multi-track waveform behavior.

User-confirmed items from earlier testing:
- waveform visibility/look improved and the bottom waveform/timeline bar was considered good
- the multi-monitor Play/Pause geometry test after `8fb399583` passed on left and right secondary monitors

Do not overstate validation:
- the `.2` release package received automated/local smoke verification
- the waveform window-growth UX fix has regression/CI coverage, but if the user now reports a visual edge case, inspect it directly
- runtime NVDEC use was not proven
- final save-sound behavior should be treated as source/behavioral fix unless user explicitly reconfirms audio on their current machine.
## Documentation / release notes state

`PATCH_NOTES.md` contains the `.2` release notes covering:
- window and playback fixes
- current-monitor / mixed-DPI behavior
- waveform Tracks/Channels outer-window growth
- OpenGL runtime fallback
- timeline fine seek
- silent save-success notification
- portable Windows packaging
- compile-time NVENC/NVDEC support
- macOS ARM64 packaging reliability and validation

`README.md` now describes Windows x64 portable and macOS 14+ Apple Silicon downloads via the latest GitHub Release, and documents the portable executable/settings behavior and macOS ad-hoc signing caveat.

The GitHub Release body was generated from the `v2.8.2-waveform.2` section of `PATCH_NOTES.md` rather than manually inventing a separate changelog.
## Known failure modes / lessons learned

1. Do not trust source-level geometry tests alone for mixed-DPI behavior. Actual Win32 window rects are authoritative.
2. Do not use `screenAt(frameGeometry().center())` as the first screen source on mixed-DPI Windows; prefer the window handle's `QScreen`.
3. Do not end the playback resize guard before pause/stop restoration is complete.
4. Do not make a stateful `mutable` Qt callback `const auto`; this caused an actual Windows compile failure.
5. Do not assume OpenGL preference implies runtime capability; required extension/function resolution must gate renderer selection.
6. Do not restore a modal save-success dialog without checking the Windows completion sound.
7. Do not pass `source_run_id` to the current Windows postprocess workflow; it has no such input.
8. Do not reuse a fixed DMG volume identity in repeated macOS packaging/smoke CI.
9. Do not claim NVDEC runtime validation based only on compile configuration.
10. Do not publish a release while one platform's final build is red/cancelled; `.2` was held until macOS passed.
## Recommended workflow for the next Avidemux change

1. Start from `/Users/flyzai/agent-workspaces/avidemux-waveform-work` and read this handoff.
2. `git fetch origin` and confirm current `master`/release state before editing.
3. Reproduce the reported symptom first when it is a runtime/UI/driver issue.
4. Add or strengthen a focused regression test before implementation when practical.
5. Run all `tests/test_*.py` plus `git diff --check` before committing.
6. Use commit message triggers intentionally:
   - `[nativewin]` when a fresh Windows native/package build is required
   - `[macarm]` when a fresh macOS ARM64 build is required
   - both when a cross-platform change needs both
7. For Windows release packaging, dispatch `windows-postprocess.yml` only after the intended native build is successful.
8. Verify the workflow's artifact `head_sha` matches the intended commit; do not accidentally package an older successful run.
9. For user-visible runtime fixes, test the freshly built executable rather than only reporting CI.
10. Create a new release version for new shipped behavior instead of silently replacing `.2` assets.
## What is complete vs. intentionally not claimed

Complete:
- waveform + marker feature line is on `master`
- `.2` patch notes and README are in repo
- Windows native CI for final release SHA is green
- Windows final portable postprocess for final release SHA is green
- macOS ARM64 CI for final release SHA is green
- `.2` GitHub Release is published with Windows + macOS assets
- release assets were re-downloaded and checksum-verified after publishing
- exact `.2` Windows release is deployed on Mainframe with settings preserved
- old Mainframe Avidemux test/build folders from the debugging cycle were previously cleaned, freeing about 1.67 GB

Not claimed / not necessary for release:
- real runtime NVDEC decode utilization was not verified
- Deniz's exact original OpenGL driver/context was not available for post-fix reproduction; fallback logic and regression coverage are the protection
- no Intel macOS or Linux binary release is published
- the separate local `win-native-experiment` branch was not deleted because it is unmerged
## Suggested first message for a future chat

Use:

`FlyzDev/avidemux2 reposunda /Users/flyzai/agent-workspaces/avidemux-waveform-work/docs/superpowers/handoffs/2026-10-04-avidemux-waveform2-release-handoff.md dosyasını oku. v2.8.2-waveform.2 release'inden devam ediyoruz. Önce master/release/CI durumunu doğrula, sonra yeni isteğime geç.`

## User working style for this project

Mustafa expects execution and fresh evidence rather than speculative status. When asked `bitti mi?`, `çıktı mı?`, `şimdi?`, check the actual workflow/artifact/runtime state before answering. Do not say a UI, audio, monitor or playback bug is fixed merely because code compiled. When a new executable is relevant, test that executable. Keep reports concise unless a handoff or deep explanation is explicitly requested.

End of handoff.