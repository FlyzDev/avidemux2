#!/usr/bin/env python3
from pathlib import Path

toolkit = Path('avidemux/qt4/ADM_UIs/src/toolkit.cpp').read_text(encoding='utf-8')
gui = Path('avidemux/qt4/ADM_userInterfaces/ADM_gui/Q_gui2.cpp').read_text(encoding='utf-8')
header = Path('avidemux/qt4/ADM_UIs/include/ADM_toolkitQt.h').read_text(encoding='utf-8')

assert 'UI_getAvailableScreenGeometry' in header, 'shared current-screen geometry helper is missing'
assert 'QGuiApplication::screenAt' in toolkit or 'windowHandle()->screen()' in toolkit, 'screen helper does not use the window current screen'
start = gui.index('void UI_resize(uint32_t w, uint32_t h)')
end = gui.index('void UI_getMaximumPreviewSize', start)
resize = gui[start:end]
assert 'UI_getAvailableScreenGeometry' in resize, 'UI_resize still does not use current-screen geometry'
assert 'QGuiApplication::primaryScreen()->availableGeometry()' not in resize, 'UI_resize still hard-codes the primary monitor'
print('multi-monitor current-screen geometry regression: PASS')
