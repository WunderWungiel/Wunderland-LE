# Usage: python gfx/gen_tab_icons.py gfx
# Generates gfx/tab_*.svg. The phone's SVG-T engine mis-renders transforms on
# shapes, strokes and compound (holed) paths, so every shape here is a simple
# straight-edged polygon or a plain rect with absolute, rounded coordinates.
import sys, os, math
out = sys.argv[1]

def f(v):
    return ('%.2f' % v).rstrip('0').rstrip('.')

def poly(pts, dy):
    return '<path d="M' + ' L'.join(f'{f(x)} {f(y + dy)}' for x, y in pts) + ' Z"/>'

def arc(cx, cy, r, a0, a1, n):
    return [(cx + r * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
             cy + r * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]

def home(dy):
    return [poly([(24, 4), (46, 24), (42, 28.5), (24, 12), (6, 28.5), (2, 24)], dy),
            poly([(33, 6), (39, 6), (39, 20), (33, 14.5)], dy),
            poly([(24, 16.5), (39, 30), (39, 44), (29, 44), (29, 33), (19, 33), (19, 44),
                  (9, 44), (9, 30)], dy)]

def categories(dy):
    return [f'<rect x="6" y="{f(y + dy)}" width="36" height="8" rx="2.5" ry="2.5"/>'
            for y in (8, 20, 32)]

def search(dy):
    cx = cy = 19.5
    ro, ri = 14, 9.5
    shapes = []
    # Ring as two C-shaped halves (no hole), overlapping by 2 degrees at the seams
    for a0 in (-1, 179):
        a1 = a0 + 182
        pts = arc(cx, cy, ro, a0, a1, 24) + arc(cx, cy, ri, a1, a0, 24)
        shapes.append(poly(pts, dy))
    # Handle: a bar along the 45 degree diagonal with a rounded outer end
    w = 4.6                       # half thickness
    d = math.sqrt(0.5)
    ux, uy = d, d                 # along the handle
    nx, ny = -d, d                # across it
    s0, s1 = ro - 2, 28.5         # from just inside the ring to the start of the cap
    pts = [(cx + ux * s0 + nx * w, cy + uy * s0 + ny * w)]
    # rounded cap: half circle around the end point, from +n to -n through +u
    ex, ey = cx + ux * s1, cy + uy * s1
    for i in range(13):
        a = math.radians(90 - 180 * i / 12)          # angle relative to u
        px = ux * math.cos(a) + nx * math.sin(a)
        py = uy * math.cos(a) + ny * math.sin(a)
        pts.append((ex + px * w, ey + py * w))
    pts.append((cx + ux * s0 - nx * w, cy + uy * s0 - ny * w))
    shapes.append(poly(pts, dy))
    return shapes

SHAPES = {'home': home, 'categories': categories, 'search': search}
# state: (top, bottom, inset)
STATES = {
 '':   ('#737981', '#40454C', False),   # normal
 '_a': ('#4A97F0', '#1056B4', True),    # active: blue, pressed in
 '_f': ('#FFFFFF', '#D4D8DE', False),   # focused (on dark grey)
 '_fa':('#B4D9FF', '#64A8F4', False),   # focused + active
}
for name, shapes in SHAPES.items():
    for suf, (top, bot, inset) in STATES.items():
        g = []
        if inset:
            g.append('<g fill="#FFFFFF">' + ''.join(shapes(1.5)) + '</g>')
            g.append('<g fill="#0A3878">' + ''.join(shapes(-1)) + '</g>')
        g.append('<g fill="url(#g)">' + ''.join(shapes(0)) + '</g>')
        svg = ('<?xml version="1.0" encoding="UTF-8"?>\n'
               '<svg xmlns="http://www.w3.org/2000/svg" version="1.1" baseProfile="tiny" '
               'width="48" height="48" viewBox="0 0 48 48">\n'
               '<defs><linearGradient id="g" gradientUnits="userSpaceOnUse" x1="0" y1="3" x2="0" y2="45">'
               f'<stop offset="0" stop-color="{top}"/><stop offset="1" stop-color="{bot}"/>'
               '</linearGradient></defs>\n' + '\n'.join(g) + '\n</svg>\n')
        open(os.path.join(out, f'tab_{name}{suf}.svg'), 'w', newline='\n').write(svg)
