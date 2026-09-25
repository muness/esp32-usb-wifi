#!/usr/bin/env python3
"""Equivalent 160x80 bounds preview from the production text formatter.
Synthetic states; a browser monospace approximation, not an LVGL screenshot.
"""
from pathlib import Path
from html import escape
blocks = Path('build-host/preview.txt').read_text().split('PAGE ')[1:]
for block in blocks:
    rows = block.splitlines()
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="640" height="320" viewBox="0 0 160 80">', '<rect width="160" height="80" fill="black"/>', '<defs><clipPath id="screen"><rect x="2" width="156" height="80"/></clipPath></defs>', '<g clip-path="url(#screen)" fill="white" font-family="monospace" font-size="9">']
    for i, line in enumerate(rows[1:6]):
        assert len(line) <= 26
        svg.append(f'<text x="2" y="{11+i*15}">{escape(line)}</text>')
    svg += ['</g></svg>']
    Path(f'docs/preview-{rows[0]}.svg').write_text('\n'.join(svg))
print('PASS: five synthetic 160x80 layout previews')
