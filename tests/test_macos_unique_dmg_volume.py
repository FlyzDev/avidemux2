from pathlib import Path

workflow = Path('.github/workflows/macos-arm64-waveform-build.yml').read_text()

assert 'PACKAGE_NAME="Avidemux-Waveform-macOS-Apple-Silicon-${GITHUB_RUN_ID}-${GITHUB_RUN_ATTEMPT}"' in workflow, 'macOS CI must use a unique internal DMG volume/package name per run'
assert '--output="$PACKAGE_NAME"' in workflow, 'bootstrap must receive the unique internal package name'
assert 'Avidemux-Waveform-macOS-Apple-Silicon.dmg' in workflow, 'published DMG filename must remain stable'
print('macOS unique DMG volume regression: PASS')
