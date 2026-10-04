#!/usr/bin/env python3
from pathlib import Path

preview = Path('avidemux/qt4/ADM_userInterfaces/ADM_gui/T_preview.cpp').read_text(encoding='utf-8')

start = preview.index('void UI_updateDrawWindowSize(void *win, uint32_t w, uint32_t h)')
end = preview.index('static void *myDisplay', start)
body = preview[start:end]

assert '#include "avi_vars.h"' in preview, 'preview code cannot observe playback state'
assert 'preserveMainWindowGeometryDuringPlayback' in body, 'playback resize guard is missing'
assert 'if (playing)' in body, 'playback resize guard is not activated by playback'
assert 'if (!preserveMainWindowGeometryDuringPlayback)' in body, 'top-level UI_resize is not gated during playback'
assert 'UI_resize(w, h);' in body, 'normal non-playback preview resize behavior was removed'
assert 'if (!playing)' in body and 'preserveMainWindowGeometryDuringPlayback = false' in body, 'playback resize guard is not cleared after pause/stop restore'
print('playback preserves user window geometry regression: PASS')
