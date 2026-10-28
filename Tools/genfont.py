# 日本語の文字をフォント画像に追加するツール
#   python Tools/genfont.py [日本語フォントのファイル]
# ソースコード（*.cpp / *.h）の文字列の中から日本語の文字を集め、
# Resources/font/font.png の英数字（1〜4行目）の下に描き足して、FontGlyphs.h に文字の並びを書き出す
# 文字列に新しい日本語の文字を使ったら、このツールを実行し直す（フォントにない文字は画面に出ない）
# 必要なもの：Python 3 と Pillow（pip install pillow）
import glob, os, re, sys
from PIL import Image, ImageChops, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONT_PNG = os.path.join(ROOT, 'Resources', 'font', 'font.png')
HEADER = os.path.join(ROOT, 'FontGlyphs.h')
CELL, COLUMNS, ASCII_ROWS = 32, 16, 4
# 日本語の文字の大きさ（ピクセル）
GLYPH_SIZE = 29
# 日本語フォントの候補（引数で指定しなければ、見つかったものを使う）
FONT_CANDIDATES = [
    '/usr/share/fonts/opentype/ipafont-gothic/ipag.ttf',  # IPAゴシック（今の font.png はこれで作った）
    'C:/Windows/Fonts/ipag.ttf',
    'C:/Windows/Fonts/meiryo.ttc',
    'C:/Windows/Fonts/msgothic.ttc',
]

def collect_chars():
    chars = set()
    for path in glob.glob(os.path.join(ROOT, '*.cpp')) + glob.glob(os.path.join(ROOT, '*.h')):
        code = open(path, encoding='utf-8').read()
        # コメントを消してから、"..." の文字列だけを取り出す
        code = re.sub(r'/\*.*?\*/', '', code, flags=re.S)
        code = re.sub(r'//[^\n]*', '', code)
        for literal in re.findall(r'"((?:[^"\\\n]|\\.)*)"', code):
            chars.update(c for c in literal if ord(c) >= 0x80)
    chars.discard('\u3000')  # 全角の空白は描かない（送り幅だけ空ける）
    return sorted(chars)

def main():
    font_path = sys.argv[1] if len(sys.argv) > 1 else next((p for p in FONT_CANDIDATES if os.path.exists(p)), None)
    if not font_path:
        sys.exit('日本語フォントが見つかりません。フォントのファイルを引数で指定してください')
    chars = collect_chars()
    font = ImageFont.truetype(font_path, GLYPH_SIZE)

    old = Image.open(FONT_PNG).convert('RGBA')
    rows = ASCII_ROWS + (len(chars) + COLUMNS - 1) // COLUMNS
    atlas = Image.new('RGBA', (CELL * COLUMNS, CELL * rows), (0, 0, 0, 0))
    # 英数字のドット文字はそのまま残す
    atlas.paste(old.crop((0, 0, CELL * COLUMNS, CELL * ASCII_ROWS)), (0, 0))
    for i, c in enumerate(chars):
        cell = Image.new('L', (CELL, CELL), 0)
        draw = ImageDraw.Draw(cell)
        # マスのまん中に描く
        box = draw.textbbox((0, 0), c, font=font)
        x = (CELL - (box[2] - box[0])) // 2 - box[0]
        y = (CELL - GLYPH_SIZE) // 2 - 1
        draw.text((x, y), c, font=font, fill=255)
        # 横に1ピクセルずらして重ね、少し太くする（小さく表示してもつぶれにくく、ドットの英数字と並んでも細く見えない）
        shifted = Image.new('L', (CELL, CELL), 0)
        shifted.paste(cell, (1, 0))
        cell = ImageChops.lighter(cell, shifted)
        glyph = Image.new('RGBA', (CELL, CELL), (255, 255, 255, 0))
        glyph.putalpha(cell)
        index = ASCII_ROWS * COLUMNS + i
        atlas.paste(glyph, ((index % COLUMNS) * CELL, (index // COLUMNS) * CELL))
    atlas.save(FONT_PNG)

    with open(HEADER, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n')
        f.write('// Tools/genfont.py が作ったファイル（手で書き換えない）\n')
        f.write('// font.png の5行目から並んでいる日本語の文字（Unicode の番号の小さい順）\n')
        f.write('inline constexpr char32_t kFontJapaneseGlyphs[] = {\n')
        for k in range(0, len(chars), 16):
            f.write('    ' + ' '.join("U'%s'," % c for c in chars[k:k + 16]) + '\n')
        f.write('};\n')
    print('%d 文字を font.png に描きました（%s）' % (len(chars), os.path.basename(font_path)))

if __name__ == '__main__':
    main()
