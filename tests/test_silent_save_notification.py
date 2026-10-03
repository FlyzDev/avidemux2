#!/usr/bin/env python3
from pathlib import Path

src = Path('avidemux/qt4/ADM_userInterfaces/ADM_dialog/alert_qt4.cpp').read_text(encoding='utf-8')

assert 'silentSuccessfulSave' in src, 'successful-save detection is missing'
assert 'showSilentSuccessfulSave(primary, secondary_format);' in src, (
    'successful save must use the dedicated non-QMessageBox dialog path'
)
assert 'alertCommon(QMessageBox::Information,\n        QT_TRANSLATE_NOOP("qtalert","Info"),\n        primary, secondary_format, silentSuccessfulSave);' not in src, (
    'successful save is still routed through QMessageBox and can trigger the Windows alert sound'
)
print('silent save notification source regression: PASS')
