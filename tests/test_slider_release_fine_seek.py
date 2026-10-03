#!/usr/bin/env python3
from pathlib import Path
import re

src = Path('avidemux/qt4/ADM_userInterfaces/ADM_gui/Q_gui2.cpp').read_text(encoding='utf-8')
match = re.search(r'void MainWindow::sliderReleased\(void\)\s*\{(?P<body>.*?)\n\}', src, re.S)
assert match, 'sliderReleased() not found'
body = match.group('body')
assert 'if (!dragWhilePlay)\n        sendAction(ACT_FineScale);' in body, (
    'mouse release should finish with frame-accurate fine seek when not dragging during playback'
)
assert 'if (!dragWhilePlay && ctrlKeyHeld)' not in body, (
    'fine seek on release must not require Ctrl'
)
print('slider release fine-seek regression: PASS')
