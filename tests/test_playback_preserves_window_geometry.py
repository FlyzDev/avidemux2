#!/usr/bin/env python3
from pathlib import Path

play = Path('avidemux/common/gui_play.cpp').read_text(encoding='utf-8')
preview = Path('avidemux/qt4/ADM_userInterfaces/ADM_gui/T_preview.cpp').read_text(encoding='utf-8')
qt_gui = Path('avidemux/qt4/ADM_userInterfaces/ADM_gui/Q_gui2.cpp').read_text(encoding='utf-8')
qt_render = Path('avidemux/qt4/ADM_render_qt/GUI_render.h').read_text(encoding='utf-8')
cli_render = Path('avidemux/cli/ADM_render_cli/GUI_render.h').read_text(encoding='utf-8')
cli_gui = Path('avidemux/cli/ADM_userInterfaces/ADM_gui2/gui_none.cpp').read_text(encoding='utf-8')

for text, name in [(qt_render, 'Qt render header'), (cli_render, 'CLI render header')]:
    assert 'UI_setBlockResizingFlag(bool block)' in text, f'{name} lacks playback resize guard API'
    assert 'UI_getBlockResizingFlag(void)' in text, f'{name} lacks playback resize guard query'
assert 'void UI_setBlockResizingFlag(bool block)' in qt_gui, 'Qt UI does not expose resize guard setter'
assert 'bool UI_getBlockResizingFlag(void)' in qt_gui, 'Qt UI does not expose resize guard getter'
assert 'void UI_setBlockResizingFlag(bool block)' in cli_gui, 'CLI UI lacks no-op resize guard setter'
assert 'bool UI_getBlockResizingFlag(void)' in cli_gui, 'CLI UI lacks no-op resize guard getter'

start = play.index('void GUI_PlayAvi(bool quit)')
end = play.index('\n}', start) + 2
body = play[start:end]
block_on = body.index('UI_setBlockResizingFlag(true);')
playing_on = body.index('playing = 1;')
playing_off = body.index('playing = 0;')
restore_display = body.index('admPreview::setMainDimension(info.width, info.height, oldZoom);')
block_off = body.rindex('UI_setBlockResizingFlag(false);')
assert block_on < playing_on, 'resize guard must be active before playback starts'
assert playing_off < restore_display < block_off, 'resize guard must remain active through pause/stop display restore'

pstart = preview.index('void UI_updateDrawWindowSize(void *win, uint32_t w, uint32_t h)')
pend = preview.index('static void *myDisplay', pstart)
pbody = preview[pstart:pend]
assert 'UI_getBlockResizingFlag()' in pbody, 'preview resizing does not honor the explicit playback resize guard'
assert 'preserveMainWindowGeometryDuringPlayback' not in pbody, 'timing-based playback resize guard should be removed'
assert 'UI_resize(w, h);' in pbody, 'normal non-playback preview resize behavior was removed'
print('playback preserves user window geometry regression: PASS')
