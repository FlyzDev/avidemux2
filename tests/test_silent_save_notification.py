#!/usr/bin/env python3
from pathlib import Path

save_src = Path('avidemux/common/gui_save.cpp').read_text(encoding='utf-8')
start = save_src.index('int A_SaveWrapper(const char *name)')
end = save_src.index('\n}', start) + 2
wrapper = save_src[start:end]

assert 'UI_notifyInfo(' in wrapper, (
    'successful save must use the non-modal status-bar notification path'
)
assert 'GUI_Info_HIG(' not in wrapper, (
    'successful save still creates a modal dialog, which can trigger the Windows notification sound'
)
print('silent save notification source regression: PASS')
