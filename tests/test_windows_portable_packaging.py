#!/usr/bin/env python3
from pathlib import Path

wf = Path('.github/workflows/windows-postprocess.yml').read_text(encoding='utf-8')
assert 'avidemux_portable.exe' in wf, 'portable package must rename the GUI executable to trigger Avidemux portable mode'
assert 'Portable mode' in wf, 'portable smoke test must verify that portable mode actually activated'
print('Windows portable packaging regression: PASS')
