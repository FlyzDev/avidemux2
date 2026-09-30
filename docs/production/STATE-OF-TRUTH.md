# Avidemux Waveform — State of Truth

Last updated: 2026-09-30

This file is the compact source of truth for the `FlyzDev/avidemux2` waveform fork. Detailed continuation notes live in `docs/production/HANDOFF-2026-09-30-WAVEFORM-MARKERS.md`.

## Repository

- GitHub: `FlyzDev/avidemux2`
- Upstream: `mean00/avidemux2`
- Active/default branch: `master`
- Marker import landed in `2edf751` (`[nativewin][macarm] feat: import timeline markers`).
- Robust imported-marker seeking fix: `2bd0229` (`[nativewin][macarm] fix: robust imported marker seeking`).
- macOS ad-hoc verification fix: `34fc929` (`[macarm] ci: validate upstream-style ad-hoc bundle`).
- Deterministic marker-seek fixture: `f3bb02c` (`test: add imported marker seek regression fixture`).
- Local Mac mini workspace: `/Users/flyzai/agent-workspaces/avidemux-waveform-work`
- `feat/waveform-ui` was fast-forward merged to `master` at `9a608f0`.

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

## P0 — resolved: imported marker navigation seek error

Resolved in `2bd0229`.

Root cause:

- Imported marker navigation called `GUI_GoToTime(target)`, which requires an exact decodable frame PTS.
- Premiere/XMEML marker times are timeline positions and may fall between actual frame PTS values.
- On 29.97 fps media this produced modal errors such as `Error seeking to 15000 ms` / `75000 ms`.

Fix:

- Imported marker navigation now uses `GUI_GoToTimeNearestFrame`.
- It first tries exact seek, then falls back via previous/next keyframes and approaches the requested timeline time from a decodable point.
- Hard failure restores the previous PTS.
- Existing A/B marker navigation behavior was left unchanged.

Deterministic Windows validation:

- Generated 180 s H.264 at `30000/1001` fps with a long GOP.
- Imported markers at 15 s / 45 s / 75 s / 105 s / 150 s.
- Old package consistently reproduced the modal.
- Final portable package passed 5/5 forward and 5/5 backward with no modal.
- Nearest-frame landings were `15.015`, `45.011`, `75.008`, `105.004`, `150.016` seconds.
- Fixture: `docs/production/fixtures/MARKER-SEEK-REPRO.md`.

## Windows build status

Final native package:

- Workflow: `Windows native waveform package`
- Run: `36682955126`
- Result: success
- Code head: `2bd0229`
- Artifact ID: `11083627270`
- Artifact name: `avidemux-waveform-windows-native-x64`
- Artifact digest: `sha256:319feaaf03f3fb90c340fd7054c4ecbec89f148eed28620690af76ad4875d630`

Final portable package:

- Workflow: `Windows waveform portable postprocess`
- Run: `36685360993`
- Result: success
- Dispatch head: `f3bb02c`
- Source native package: successful `2bd0229` native artifact above.
- Artifact ID: `11083855798`
- Artifact name: `avidemux-waveform-windows-portable-x64`
- Artifact digest: `sha256:701552f78dc76e4bc86126708efc6ce7603404143affa592340a6225537efdb7`
- Inner portable ZIP SHA-256: `EF8A25B4D9975E3BB45AD39EC29B360FAF22EECA2E95DDF48C17EB90E705D88B`
- Dependency scan and generated multi-track smoke launch passed.
- Mainframe real-GUI imported-marker regression passed in both directions.

Mainframe device:

- `cf5c57b1-3c3a-44ee-81a6-4f6393fdb4be`

## macOS Apple Silicon status

Apple Silicon packaging is green and release-validated.

- Minimum OS: **macOS 14.0**.
- Fix commit: `c987bb5` (`[macarm] fix: make macOS bundle LaunchServices compatible`).
- Workflow: `macOS ARM64 waveform build`
- Final run: `36689839945`
- Result: success
- Head: `c987bb505aad24f1b24c4ffab78c3921f3aae827`
- Artifact ID: `11085504140`
- Artifact name: `avidemux-waveform-macos-arm64`
- Artifact digest: `sha256:a5609240759b4d3515f22162382da2a42dcb50d0529cf5372f6e9227eeada4a7`
- Final DMG SHA-256: `c8f1aeb2c815f24131c9d0cda92f8ea10edf14d8752d5a99f23270b49378442c`
- Final app ZIP SHA-256: `26f9d780e75b1b4b3e43118ddc5665dacb805715a4f6dfdc81882ddf328642ce`
- Root cause of the first public macOS asset issue: the bootstrap script overwrote `MACOSX_DEPLOYMENT_TARGET` with the newest installed SDK version, producing a main binary with minimum macOS 26.4 on a macOS 26.2 runner; the Qt framework cleanup also removed framework symlinks needed for a valid bundle.
- Bootstrap now honors an externally supplied deployment target; CI pins it to macOS 14.0, matching bundled Homebrew Qt requirements.
- Qt framework `Versions/Current` and top-level binary symlinks are preserved/restored.
- The full application bundle is ad-hoc signed and passes `codesign --verify --deep --strict`.
- `hdiutil verify` passes.
- The final DMG was mounted and the contained app was launched through LaunchServices (`open -na`), matching Finder-style launch behavior.
- The published Preview 1 macOS assets were replaced with this corrected, validated build.

Mac mini runner / workspace:

- Device: `e839f56f-5a89-4f77-a40e-abaa5ac162f4`
- Workspace: `/Users/flyzai/agent-workspaces/avidemux-waveform-work`
- Self-hosted runner: `avidemux-waveform-mac-arm64`

## GitHub / public release status

Preview 1 is published.

- Release: `Avidemux Waveform 2.8.2 – Preview 1`
- Tag: `v2.8.2-waveform.1`
- Tag target: `c987bb505aad24f1b24c4ffab78c3921f3aae827`
- Published as GitHub prerelease on 2026-09-30.
- Windows asset: `Avidemux-Waveform-Windows-x64-Portable.zip`
- macOS assets: `Avidemux-Waveform-macOS-Apple-Silicon.dmg` and `Avidemux-Waveform-macOS-Apple-Silicon.app.zip` (final macOS 14+ LaunchServices-validated replacements)
- Checksum manifest: `SHA256SUMS.txt`
- Public release URL: https://github.com/FlyzDev/avidemux2/releases/tag/v2.8.2-waveform.1

## Next order of work

1. Collect Preview 1 user feedback and crash/hang reports.
2. Fix any release-blocking regressions before expanding marker/waveform scope.
3. Keep Windows portable and macOS ARM64 packaging workflows green for subsequent previews.
4. Consider Intel macOS / Linux binaries only after the current Windows + Apple Silicon preview stabilizes.
