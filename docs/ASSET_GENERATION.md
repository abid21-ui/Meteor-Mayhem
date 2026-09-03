# Asset-generation brief

The rebuilt art uses the supplied 256×256 sprite collection as a visual anchor.
Each output was requested as one isolated sprite with a genuine transparent
background, no text, border, shadow, or watermark.

## Reference-derived sprites

- **Player ship:** Recreate the small teal-and-green upright rocket near the
  upper-left as a symmetric, clean retro pixel-art player craft with navy
  outlines and small orange engine accents, readable at 64 pixels.
- **Striker ship:** Using the supplied sprite sheet as the reference, recreate
  the slim red-and-white upright rocket from the second row as one centered,
  symmetric retro pixel-art craft. Preserve its red fins, white fuselage, dark
  outline, and small blue details; use a truly transparent background with no
  text, border, shadow, or watermark.
- **Comet ship:** Using the supplied sprite sheet as the reference, recreate
  the broad blue-and-orange upright craft from the second row as one centered,
  symmetric retro pixel-art ship. Preserve the blue body, orange side pods,
  dark outline, and compact silhouette; use a truly transparent background
  with no text, border, shadow, or watermark.
- **Alien ship:** Recreate the large brown armored ship below the smaller craft
  as a bulky, symmetric alien battleship with bronze armor and blue details,
  readable at 80 pixels.
- **Asteroid:** Recreate the large round cratered brown asteroid near the
  bottom-left as a single irregular, warm-umber space rock with a strong
  pixel-art silhouette, readable at 64 pixels.

## New gameplay sprites

- **Laser:** One narrow upward cyan-blue bolt with a white core, deep-blue pixel
  edge, and controlled glow, readable at 24 pixels.
- **Scattershot pellet:** One compact amber-orange diamond with a pale-yellow
  center and burnt-orange edge, readable at 12 pixels and clearly different
  from the laser.
- **Shield aura:** One hollow circular force-field ring with segmented cyan arcs,
  electric sparks, transparent interior, and subtle blue glow, sized to overlay
  around the player ship.

The two alternate ships were created with OpenAI image generation, then trimmed,
resized, and centered on transparent 128x128 canvases. Final runtime assets are
`assets/player_ship_red.png` and `assets/player_ship_blue.png`; the original
green craft is preserved as `assets/player_ship_green.png`.

All final project assets were stored under `assets/` without overwriting the
generated masters.
