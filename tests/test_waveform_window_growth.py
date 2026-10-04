from pathlib import Path

src = Path('avidemux/qt4/ADM_userInterfaces/ADM_gui/Q_gui2.cpp').read_text()
anchor = src.index('// Waveform overview.')
block = src[anchor:anchor + 3200]

assert 'previousWaveformHeight' in block, 'waveform resize handler must track the previous preferred height'
assert 'const int delta = waveformHeight - previousWaveformHeight' in block, 'main-window growth must be based on the waveform height delta'
assert 'isMaximized()' in block and 'isFullScreen()' in block, 'maximized/fullscreen windows must not be force-resized'
assert 'UI_getAvailableScreenGeometry(this)' in block, 'waveform growth must use the current monitor work area'
assert 'resize(width(), height() + appliedDelta)' in block, 'main window must grow/shrink by the applied waveform delta'
assert 'setBlockZoomChangesFlag(true)' in block and 'setBlockZoomChangesFlag(false)' in block, 'waveform-driven outer resize must not alter video zoom'
print('waveform window-growth regression: PASS')
