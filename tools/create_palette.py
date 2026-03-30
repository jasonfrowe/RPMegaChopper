#!/usr/bin/env python3
from PIL import Image, ImageDraw

# IRGB Palette Generation
# This generates a palette compatible with the convert_sprite.py tile converter.
#
# Logic:
# Dim Colors (Indices 0-7): 0 or 170 (0xAA)
# Bright Colors (Indices 8-15): 0 or 255 (0xFF)
#
# Note: Index 8 (Bright Black) maps to Black (0,0,0) with this logic, 
# effectively giving two blacks, which is common in some IRGB implementations.

colors = []

# Dim colors (I=0)
# Bit 0: Red, Bit 1: Green, Bit 2: Blue
for i in range(8):
    r = 170 if (i & 1) else 0
    g = 170 if (i & 2) else 0
    b = 170 if (i & 4) else 0
    colors.append((r, g, b))

# Bright colors (I=1)
for i in range(8):
    r = 255 if (i & 1) else 0
    g = 255 if (i & 2) else 0
    b = 255 if (i & 4) else 0
    colors.append((r, g, b))

# Create image
# 8 columns, 2 rows.
block_size = 32
img_width = block_size * 8
img_height = block_size * 2

img = Image.new('RGB', (img_width, img_height))
draw = ImageDraw.Draw(img)

for idx, color in enumerate(colors):
    x = (idx % 8) * block_size
    y = (idx // 8) * block_size
    draw.rectangle([x, y, x + block_size, y + block_size], fill=color)
    
    # Optional: Add text label (requires font, skipping for simplicity, just blocks)

output_path = "images/palette.png"
img.save(output_path)
print(f"Created {output_path}")
