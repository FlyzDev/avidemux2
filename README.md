# Avidemux Waveform + Markers

A cross-platform community fork of [Avidemux](https://github.com/mean00/avidemux2) for fast copy-mode editing, with cached audio waveforms, Premiere/XMEML/CSV/JSON timeline marker import, and ready-to-run Windows x64 + macOS Apple Silicon builds.

[![Latest release](https://img.shields.io/github/v/release/FlyzDev/avidemux2?display_name=tag&sort=semver)](https://github.com/FlyzDev/avidemux2/releases/latest)
[![Windows build](https://github.com/FlyzDev/avidemux2/actions/workflows/windows-native-package.yml/badge.svg?branch=master)](https://github.com/FlyzDev/avidemux2/actions/workflows/windows-native-package.yml)
[![macOS ARM64 build](https://github.com/FlyzDev/avidemux2/actions/workflows/macos-arm64-waveform-build.yml/badge.svg?branch=master)](https://github.com/FlyzDev/avidemux2/actions/workflows/macos-arm64-waveform-build.yml)

## Why this fork exists

Avidemux is excellent for quick cuts and stream-copy exports, but the stock timeline does not show audio and has limited interchange for editor markers. This fork keeps the lightweight workflow while adding visual audio navigation and timeline marker import for faster review, cutting and handoff.

The waveform is generated in the background. You can continue seeking and working while it fills in, and cached peak data is reused the next time the same edit state is opened.

## Download

| Platform | Build | Download |
| --- | --- | --- |
| Windows 10/11 x64 | Portable | [Latest release](https://github.com/FlyzDev/avidemux2/releases) |
| macOS 14+ Apple Silicon | DMG / app zip | [Latest release](https://github.com/FlyzDev/avidemux2/releases) |
| Source | Git | [Releases](https://github.com/FlyzDev/avidemux2/releases) |

Intel macOS and Linux binaries are not published yet. The source remains cross-platform.

## Waveform features

- Combined overview by default.
- Right-click the waveform for separate audio tracks or separate channels.
- Progressive background waveform generation; the UI remains usable while indexing.
- Low-priority waveform worker so playback and seeking keep priority.
- Cached peak data for fast reopen.
- Multi-track and multi-channel audio support.
- Playhead plus A/B selection markers drawn directly over the waveform.
- Left-click waveform seeking.
- Background demux/index work is prevented from opening modal progress UI, avoiding cross-thread GUI deadlocks.
- Existing Avidemux copy-mode editing and export behavior is preserved.
- Timeline marker import from Premiere / XMEML XML, CSV and JSON.
- Imported markers are drawn on both the seek bar and waveform.
- Dedicated previous / next marker navigation buttons and keyboard shortcuts.

## Install

### Windows

Download the Windows x64 portable zip from the latest release, extract it to a normal folder, then run `avidemux.exe`.

No installer is required. Keep the DLLs and plugin folders next to the executable.

### macOS Apple Silicon

Download the Apple Silicon DMG from the latest release and drag the app to Applications. The community build is ad-hoc signed but is not Apple-notarized, so macOS may require **Right click → Open** the first time.

The current macOS binary targets **macOS 14+** on Apple Silicon (M1/M2/M3/M4 and newer ARM64 Macs). The release DMG is checksum-verified, strict ad-hoc code-signature verified, and smoke-tested by mounting the DMG and launching the packaged app through macOS LaunchServices.

## Using the waveform

Open a video normally. The waveform area appears under the seek bar and starts filling in shortly after the file becomes usable. You do not need to wait for waveform generation before seeking or editing.

Right-click the waveform to switch between:

- **Master / combined waveform** — compact overview of active audio tracks.
- **Separate audio tracks** — one lane per audio track.
- **Separate channels** — individual channel lanes for each track.

For long or multi-track files, the first pass can take a little time. The result is cached and reused when the source/timeline state matches.

## Timeline markers

Use the **M+** button beside the navigation controls to import timeline markers. The importer accepts:

- Premiere / Final Cut Pro style XMEML XML sequence markers (`<marker><name>…</name><in>frame</in>`), using the sequence timebase / NTSC flag.
- CSV files including common columns such as `Marker Name`, `In`, `Time`, `Timecode`, `Frame`, `Seconds`, `Description` and `FPS`.
- JSON as either an array of marker objects or `{ "fps": 60, "markers": [...] }`. Marker time can be supplied as `timeUs`, `timeMs`, `seconds`, `frame`, `in`, `time`, or `timecode`.

Imported markers are session-local and do not replace Avidemux's A/B selection markers. They appear as marker lines on the seek bar and waveform. Use **M◀ / M▶** or **Alt+Left / Alt+Right** to jump to the previous or next imported marker.

## Status

This is an unofficial community release. The Windows build is tested with generated multi-track media, portable dependency scanning, and a deterministic imported-marker navigation regression on 29.97 fps media. The macOS build targets macOS 14+ on Apple Silicon, verifies DMG integrity and strict bundle signing, and is smoke-tested through LaunchServices before publishing.

If you hit a hang or crash, open an issue with the source container/codec details and, when available, the Windows WER/ProcDump report or macOS crash report.

## Build from source

The waveform work lives in this repository on top of upstream Avidemux. Avidemux build directories must be on a case-sensitive filesystem on macOS.

### macOS Apple Silicon

Install the normal Avidemux build dependencies with Homebrew, then run:

```bash
bash bootStrapMacOS_Monterey.arm64.sh --with-internal-libmp4v2
```

### Windows

The repository contains GitHub Actions workflows for a native MSYS2 UCRT64 build and portable package validation. See `.github/workflows/windows-native-package.yml` and `.github/workflows/windows-postprocess.yml`.

For the full upstream build documentation and supported platforms, see the [official Avidemux repository](https://github.com/mean00/avidemux2).

## Upstream and license

This is an unofficial community fork, not an official Avidemux release. Avidemux and the overwhelming majority of this codebase are maintained by the upstream Avidemux project and contributors.

The project remains under the licenses used by upstream Avidemux. See the repository license files and source headers for details.
