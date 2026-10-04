from pathlib import Path

root = Path(__file__).resolve().parents[1]
factory = root / 'avidemux/qt4/ADM_render_qt/GUI_renderFactory_common.cpp'
text = factory.read_text(encoding='utf-8')

case = text.index('case RENDER_QTOPENGL')
end = text.index('break;', case)
block = text[case:end]

assert 'ADM_glHasActiveTexture()' in block, 'QtGL renderer selection must reject missing glActiveTexture at runtime'
assert block.index('ADM_glHasActiveTexture()') < block.index('TRY_RENDERER_SPAWN_ALL'), 'runtime GL capability check must happen before spawning QtGL renderer'
print('OpenGL runtime fallback regression: PASS')
