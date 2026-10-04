# Avidemux Waveform + Markers — Patch Notes

## v2.8.2-waveform.2

This patch release focuses on Windows/macOS usability, display stability and packaging reliability on top of `v2.8.2-waveform.1`.

### Window and playback fixes

- Play / Pause no longer changes the user-selected top-level window size.
- Playback keeps the window on its current monitor instead of snapping toward the primary display.
- Mixed-DPI multi-monitor setups prefer the window's actual `QScreen`, avoiding coordinate-space mismatches.
- Expanding waveform views to Tracks or Channels grows the application window when room is available instead of covering the video preview. Maximized and fullscreen windows are left alone.

### Rendering and timeline fixes

- OpenGL display now falls back safely when a required runtime extension such as `glActiveTexture` is unavailable, instead of asserting during first-frame upload.
- Timeline dragging remains fast/keyframe-oriented, while mouse release performs the fine seek expected for accurate positioning.
- Waveform visibility/scaling was improved for easier reading.

### Windows behavior and packaging

- Successful saves use a non-modal status notification, avoiding the unwanted completion sound from modal dialogs. Save failures still surface as errors.
- The portable package now explicitly ships `avidemux_portable.exe` and uses a local `settings` directory.
- Native Windows builds detect `ffnvcodec` correctly and compile NVENC/NVDEC support when available.
- Portable smoke tests explicitly select the portable GUI executable and validate startup with multi-track media.

### macOS Apple Silicon

- macOS ARM64 release builds continue to target macOS 14+.
- Packaging cleans up stale mounted test DMGs before rebuilding and uses a unique internal DMG volume name per CI run, avoiding macOS App Management / LaunchServices reuse failures after smoke-launching a prior build.
- Release validation includes strict bundle signing checks, DMG verification, ARM64/minimum-OS checks and a LaunchServices smoke launch with multi-track media.

### Validation

- Source regressions cover multi-monitor geometry, playback window preservation, OpenGL fallback, save notifications, fine seek, waveform-driven window growth and Windows portable packaging.
- Windows native and portable packages are smoke-tested with generated multi-track media before release.
- macOS release artifacts are produced by the ARM64 workflow and verified before publishing.

### Notes

This remains an unofficial community fork of Avidemux. Existing copy-mode workflows are preserved.
