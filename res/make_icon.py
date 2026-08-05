# -*- coding: utf-8 -*-
# ============================================================
#  鼠标右键主理人 ContextMenuMaster —— 应用图标生成
#
#  依 res/icon.svg 的 M3 概念（圆角渐变底 + 右键菜单卡片 + 光标）用 PIL 重绘，
#  各尺寸单独在高分辨率下绘制再降采样（保证 16/32 小图标边缘干净），
#  打包成多尺寸 res/app.ico。CMakeLists 通过 res/app.rc 把它嵌进 exe。
#
#  运行：python res/make_icon.py    （需 Pillow）
# ============================================================
import os
from PIL import Image, ImageDraw

OUT_DIR = os.path.dirname(os.path.abspath(__file__))
ICO_PATH = os.path.join(OUT_DIR, "app.ico")
SIZES = [16, 32, 48, 64, 128, 256]
SS = 4  # 超采样倍率，先大后缩，抗锯齿


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def rounded_rect(draw, box, r, fill, outline=None, width=1):
    # 整数化坐标，避免小尺寸下浮点让 PIL 内部椭圆盒坐标翻转报错。
    x0, y0, x1, y1 = [int(round(v)) for v in box]
    if x1 < x0 or y1 < y0:
        return
    w, h = x1 - x0, y1 - y0
    # 半径留 1px 余量：PIL rounded_rectangle 在 r 逼近半宽/半高时内部子矩形坐标会翻转。
    r = int(max(0, min(r, (w - 2) / 2.0, (h - 2) / 2.0)))
    if r < 1:
        draw.rectangle([x0, y0, x1, y1], fill=fill, outline=outline, width=width)
    else:
        draw.rounded_rectangle([x0, y0, x1, y1], radius=r, fill=fill,
                               outline=outline, width=width)


def draw_icon(px):
    """在 px×px 画布上绘制图标（内部按 SS 超采样）。"""
    S = px * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def u(v):
        # 256 基准坐标 → 当前画布
        return v / 256.0 * S

    # —— M3 圆角渐变底：竖向逐行插值品牌蓝→青紫近似（对角用竖向渐变代偿，够用） ——
    c0 = (0x4D, 0x9F, 0xFF)   # 左上蓝
    c1 = (0x3D, 0x7F, 0xE0)   # 中
    c2 = (0x6A, 0x5A, 0xE0)   # 右下青紫
    grad = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    gp = grad.load()
    for y in range(S):
        ty = y / (S - 1)
        for x in range(S):
            tx = x / (S - 1)
            t = (tx + ty) / 2.0            # 对角
            col = lerp(c0, c1, t * 2) if t < 0.5 else lerp(c1, c2, (t - 0.5) * 2)
            gp[x, y] = (col[0], col[1], col[2], 255)

    # 圆角遮罩
    mask = Image.new("L", (S, S), 0)
    md = ImageDraw.Draw(mask)
    inset = u(8)
    md.rounded_rectangle([inset, inset, S - inset, S - inset], radius=u(56), fill=255)
    img.paste(grad, (0, 0), mask)

    d = ImageDraw.Draw(img)
    # 顶部高光
    hl = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    hd = ImageDraw.Draw(hl)
    hd.rounded_rectangle([inset, inset, S - inset, u(128)], radius=u(56),
                         fill=(255, 255, 255, 26))
    img.alpha_composite(hl)

    d = ImageDraw.Draw(img)
    # —— 右键菜单卡片 —— (小尺寸描边省略，避免糊)
    card = [u(66), u(58), u(184), u(190)]
    rounded_rect(d, card, u(26), (250, 251, 253, 255))
    if px >= 32:
        cr = int(u(26))
        d.rounded_rectangle([int(round(v)) for v in card], radius=cr,
                            outline=(32, 48, 90, 40), width=max(1, int(u(1))))

    # —— 菜单条目 —— 每行 = 左侧小图标方块 + 右侧等宽文字条，行距均匀（读感是
    #    「右键菜单」而非「文档段落」）。第 2 行整条 accent 高亮 = 选中项。
    ico_dim = (0xB4, 0xC0, 0xD4, 255)     # 普通行小图标
    bar_dim = (0xC8, 0xD2, 0xE0, 255)     # 普通行文字条
    acc = (0x4D, 0x9F, 0xFF, 255)         # 选中行高亮块
    on_acc = (255, 255, 255, 255)         # 高亮块上的白图标/文字
    ico_l, ico_sz = u(80), u(14)          # 小图标左边距、边长
    txt_l, txt_r = u(104), u(170)         # 文字条左右
    rows_y = [u(80), u(106), u(132), u(158)]   # 四行等距顶边
    rh = u(13)                            # 行内元素高
    acc_idx = 1
    for i, ry in enumerate(rows_y):
        if i == acc_idx:
            # 满宽高亮块 + 白图标 + 白文字条
            rounded_rect(d, [u(74), ry - u(3), u(176), ry + rh + u(3)], u(7), acc)
            rounded_rect(d, [ico_l, ry, ico_l + ico_sz, ry + ico_sz], u(3), on_acc)
            rounded_rect(d, [txt_l, ry + u(3), u(158), ry + rh - u(2)], u(4), on_acc)
        else:
            rounded_rect(d, [ico_l, ry, ico_l + ico_sz, ry + ico_sz], u(3), ico_dim)
            r_end = txt_r if i != 3 else u(150)   # 末行略短，自然收尾
            rounded_rect(d, [txt_l, ry + u(3), r_end, ry + rh - u(2)], u(4), bar_dim)

    # —— 光标箭头 ——
    ox, oy = u(150), u(150)
    pts = [(0, 0), (0, 58), (15, 44), (26, 66), (38, 60), (27, 38), (48, 38)]
    poly = [(ox + p[0] / 256.0 * S, oy + p[1] / 256.0 * S) for p in pts]
    d.polygon(poly, fill=(255, 255, 255, 255),
              outline=(32, 48, 90, 255))
    # 描边加粗（polygon outline 只有 1px，小尺寸够；大尺寸补线）
    if px >= 48:
        d.line(poly + [poly[0]], fill=(32, 48, 90, 255), width=max(2, int(u(4))),
               joint="curve")

    return img.resize((px, px), Image.LANCZOS)


def main():
    imgs = [draw_icon(s) for s in SIZES]
    # Pillow 多尺寸 .ico：以最大图为主，sizes 列全部尺寸
    imgs[-1].save(ICO_PATH, format="ICO",
                  sizes=[(s, s) for s in SIZES])
    # 顺带导出 256 PNG 供 README / 商店用
    imgs[-1].save(os.path.join(OUT_DIR, "app-256.png"), format="PNG")
    print("wrote", ICO_PATH, "and app-256.png")


if __name__ == "__main__":
    main()
