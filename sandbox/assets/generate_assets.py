#!/usr/bin/env python3
"""Generates the sandbox's real PNG assets (deterministic, no randomness).

Outputs, relative to this script's directory:
  textures/tileset.png  256x256, a 4x4 grid of 64px cells with dark borders
                        (cell index i is at row i//4, column i%4, top-left
                        origin -- matches SpriteSheet indexing)
  textures/player.png   48x48 player sprite

Run: python3 generate_assets.py
"""

from pathlib import Path

from PIL import Image, ImageDraw

# The 16 palette colors in true RGB (the engine's 0xAABBGGRR constants are the
# little-endian view of these same colors).
PALETTE = [
    "#40342E", "#52423B", "#5E4C43", "#5A564C",
    "#BBBC8F", "#D0C088", "#C1A181", "#AC815E",
    "#6A61BF", "#7087D0", "#8BCBEB", "#8CBEA3",
    "#AD8EB4", "#80726B", "#AFA39C", "#DBD5D1",
]
BORDER = "#1C1511"

CELL = 64
BORDER_PX = 4
COLS = ROWS = 4


def make_tileset() -> Image.Image:
    size = CELL * COLS
    img = Image.new("RGBA", (size, size))
    draw = ImageDraw.Draw(img)
    for index in range(COLS * ROWS):
        col, row = index % COLS, index // COLS
        x0, y0 = col * CELL, row * CELL
        draw.rectangle([x0, y0, x0 + CELL - 1, y0 + CELL - 1], fill=PALETTE[index])
        draw.rectangle(
            [x0, y0, x0 + BORDER_PX - 1, y0 + BORDER_PX - 1], fill=BORDER
        )
        draw.rectangle(
            [x0 + CELL - BORDER_PX, y0, x0 + CELL - 1, y0 + BORDER_PX - 1], fill=BORDER
        )
        draw.rectangle(
            [x0, y0 + CELL - BORDER_PX, x0 + BORDER_PX - 1, y0 + CELL - 1], fill=BORDER
        )
        draw.rectangle(
            [
                x0 + CELL - BORDER_PX,
                y0 + CELL - BORDER_PX,
                x0 + CELL - 1,
                y0 + CELL - 1,
            ],
            fill=BORDER,
        )
    return img


def make_player() -> Image.Image:
    size = 48
    img = Image.new("RGBA", (size, size), PALETTE[12])
    draw = ImageDraw.Draw(img)
    # Face marker so rotation is visible: a darker notch toward +x.
    draw.rectangle([size - 16, size // 2 - 6, size - 5, size // 2 + 5], fill=BORDER)
    draw.rectangle([0, 0, size - 1, 3], fill=BORDER)
    draw.rectangle([0, size - 4, size - 1, size - 1], fill=BORDER)
    return img


WALK_FRAMES = 4
PLAYER = 48
LEG = "#80726B"  # darker purple-grey for the stride legs


def draw_walk_frame(draw: ImageDraw.ImageDraw, x0: int, phase: int) -> None:
    """One 48px walk frame at strip offset x0. Phase 0 = standing (idle pose);
    phases 1-3 alternate the legs, phases 1/3 bob the body up 2px."""
    bob = 2 if phase in (1, 3) else 0
    # Body.
    draw.rectangle([x0, bob, x0 + PLAYER - 1, PLAYER - 5 + bob], fill=PALETTE[12])
    # Face notch toward +x (kept in every frame so facing stays readable).
    draw.rectangle(
        [x0 + PLAYER - 16, PLAYER // 2 - 6 + bob, x0 + PLAYER - 5, PLAYER // 2 + 5 + bob],
        fill=BORDER,
    )
    # Legs: standing together on phase 0/2, split on 1, mirrored split on 3.
    if phase == 1:
        legs = [(x0 + 8, x0 + 15), (x0 + PLAYER - 16, x0 + PLAYER - 9)]
    elif phase == 3:
        legs = [(x0 + PLAYER - 16, x0 + PLAYER - 9), (x0 + 8, x0 + 15)]
    else:
        legs = [(x0 + 16, x0 + 23), (x0 + 24, x0 + 31)]
    for lx0, lx1 in legs:
        draw.rectangle([lx0, PLAYER - 6, lx1, PLAYER - 1], fill=LEG)


def make_player_walk() -> Image.Image:
    img = Image.new("RGBA", (PLAYER * WALK_FRAMES, PLAYER))
    draw = ImageDraw.Draw(img)
    for frame in range(WALK_FRAMES):
        draw_walk_frame(draw, frame * PLAYER, frame)
    return img


def main() -> None:
    out_dir = Path(__file__).resolve().parent / "textures"
    out_dir.mkdir(parents=True, exist_ok=True)
    make_tileset().save(out_dir / "tileset.png")
    make_player().save(out_dir / "player.png")
    make_player_walk().save(out_dir / "player_walk.png")
    print(f"assets written to {out_dir}")


if __name__ == "__main__":
    main()
