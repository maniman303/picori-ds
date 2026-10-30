# EU acceptance matrix (2026-10-30)

This closes the broad EU coverage item for the profiles and contexts that are
actually available. 4:3 and 32:9 remain runtime-disabled by their independent
scene, sprite, layout and profile gates and are not claimed here.

## Retained functional-screen evidence

The existing EU/English and localization audits already cover the design's
functional families:

- Pause/titled/plain/pieces/techniques/fusion menus, all 15 family/health
  combinations, the 109/130-case menu replay, and the 22 detail-map recipes:
  `.tmp/background-detail-map-actions-audit/eu-en-menus`,
  `.tmp/background-family-audit/eu-en`, and the checked task entries above it.
- Fast travel, prologue/credits, file selection/operations, Game Over, hints,
  dungeons, fusion actions, figurines, sleep, save/inventory routes, ending and
  file-write/error returns: `.tmp/background-adapter-2026-10-28/`,
  `.tmp/background-*-audit/eu-*`.
- Intermediate palette/brightness/alpha effects, title-cap composition and
  fixed-screen perimeter pixels: `.tmp/background-perimeter-2026-10-29/`,
  `.tmp/procedural-composition-2026-10-29/`,
  `.tmp/title-cap-composition-2026-10-28/`, and
  `tools/background_frame_composition_regression_test.py`.
- PNG decode/profile/cache failures and missing-art fallback:
  `tools/background_png_regression_test.py`,
  `tools/background_cache_regression_test.py`, and the fast-travel presenter
  matrix in `.tmp/fast-travel-presenters-2026-10-30/`.

## Live-gameplay matrix

`tools/verify_eu_gameplay_presenters.py` runs the EU field room through the
repro harness at four-times presentation size:

| Profiles | Scenes | Presenters |
| --- | --- | --- |
| native 960 × 640 | clean world, HUD, bottom dialogue, top dialogue | software, SDL_GPU |
| 16:9 1140 × 640 | same | software, SDL_GPU |
| 21:9 1496 × 640 | same | software, SDL_GPU |

It also runs two live profile switches while a message is open:
native → 16:9 with bottom dialogue, and 16:9 → 21:9 with top dialogue. Each run
uses a magenta replacement for the fixed fast-travel artwork; any magenta in a
gameplay capture fails the check. The verifier requires exact presentation
sizes, byte-identical software/GPU **base PPU framebuffers**, identical
center-sampled native frames from the presented windows, HUD hearts/buttons in
the top corners, and dialogue-band differences against the HUD capture.
SDL_Renderer and SDL_GPU can differ slightly at 4× block edges when the source
width is not a multiple of the block size; the underlying native frame and all
block centers are identical. Native 240-wide presentation is byte-identical.

The final run passed with 34 captures. Software and SDL_GPU base framebuffers
were byte-identical for all three profiles; center-sampled native frames from
the presented windows were identical for every scene; HUD hearts/buttons and
both dialogue bands were present; no magenta artwork appeared; both live
profile switches applied and captured at the expected window size. Evidence is
retained in `.tmp/eu-gameplay-2026-10-30/matrix-final/` with the summary in
`matrix-final.log`.

Supporting retained checks were rerun for this acceptance pass:

- `tools/background_png_regression_test.py`
- `tools/background_cache_regression_test.py`
- `tools/background_compose_regression_test.py`
- `tools/background_effect_regression_test.py`
- `tools/background_frame_composition_regression_test.py`
- `tools/background_adapter_regression_test.py`
- `tools/background_policy_regression_test.py`
- `tools/verify_fast_travel_presenters.py .tmp/fast-travel-presenters-2026-10-30`
- `tools/verify_title_cap_composition.py .tmp/title-cap-composition-2026-10-28`
- `tools/verify_objwindow_title_presenters.py .tmp/title-cap-objwindow-2026-10-29`
- `tools/verify_background_perimeter_capture.py ... --min-frame 755 --skip-frame 1057`
  (241 matching native crops)

## Regional review

No USA or JP ROM is available for captures. Source review confirms that the
semantic background catalog and filename contract are shared, artwork lives
outside the per-region asset trees, non-USA runs never fall back to the flat
USA asset tree, and JP sprite/animation resolution stays on ROM-resolved data.
EU is the tested region; USA/JP runtime behavior remains explicitly unverified.

## Limitations

- The live matrix uses the EU field room; it does not multiply every functional
  screen by every profile. The retained functional-screen audits remain the
  evidence for those contexts.
- Presented block edges can differ slightly between SDL_Renderer and SDL_GPU
  when the source width is not block-aligned; non-integer window scales (for
  example 960 × 540 for a 285-wide frame) differ more. Base-framebuffer byte
  parity and block-center native samples are asserted at the exact 4× profile
  sizes used by the acceptance captures.
- 4:3, 32:9, optional upscale/filter/voxel paths and USA/JP remain outside this
  run's tested envelope.
