# Validation record

The rebuilt source was checked with the following automated passes:

- All six C source units compile as C11 with `-Wall -Wextra -Wpedantic -Werror`
  against a strict Raylib-compatible interface.
- Menu start, Settings and Statistics navigation, ship selection, audio toggles,
  weapon switching, three starting lives, shield charging, shield activation,
  and invincibility state transitions pass an automated game-state test.
- The v2.3 gameplay checks verify the 1.5x entity/hitbox scale, 1.25-second
  post-shield invincibility window, zero shield recharge during overdrive, and
  shielded contact destruction for both alien ships and asteroids.
- Alien travel is checked against the scaled sprite bounds: each ship's computed
  stopping point is fully inside the 1280x720 playfield.
- The in-run `R` input is wired to the same clean run reset used by a new game,
  clearing all active entities and resetting score, lives, weapon, and spawning.
- The game-state test passes AddressSanitizer and UndefinedBehaviorSanitizer.
  LeakSanitizer is excluded because the validation environment does not support
  it under its process tracer.
- Settings complete a save/load round trip. Automated assertions verify that
  harder difficulties increase spawn pressure, enemy speed, and score rewards
  while reducing shield charge earned per alien.
- Lifetime statistics complete a save/load round trip, including a 64-bit total
  accumulated-points value.
- Every asset and sound path referenced by the source resolves to a bundled file.
- All eight runtime sprite PNGs retain alpha transparency after resizing.
- The 1280x720 background is stored as a standard 8-bit RGB PNG so it loads
  under Raylib's default Windows image configuration.
- All three sounds validate as 16-bit, mono, 44.1 kHz WAV files.

A graphical run still requires a desktop environment and Raylib 6.0. The CMake
configuration fetches the pinned official Raylib release when it is not already
installed.
