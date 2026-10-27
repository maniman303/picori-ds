# Background effect targets and raster continuation evidence

This records the EU source/effect audit for the next implementation work in
[the task list](aspect-ratio-and-background-overrides-tasks.md). It does not
enable a profile or publish a compositor descriptor. The only available retail
fixture is EU; USA/JP remain unverified. Static artwork references omit live
foreground OBJ/text and cannot by themselves prove every animated window or
effect transition.

## Unfaded source ownership

`tools/background_effect_coverage.py` decodes the same verified ROM groups as
the exporter and counts the winning nontransparent BG source and palette bank.
It separately records each native edge in `(layer << 4) | bank` form: layer 5
is backdrop, while layers 0–3 are the corresponding BGs. The edges describe
source ownership, not a color-key or an instruction to move native pixels.
Foreground OBJ overlays are explicitly omitted and remain native. All five EU
languages have identical layer/bank masks and edge ownership for these 33
reference recipes. Twenty recipes use more than one BG palette bank.

| Context / native reference | Unfaded source owners | Effect consequence |
| --- | --- | --- |
| Logo field | Palette-zero backdrop, bank 0; the logo contains BG2 foreground, including white index 8 | In the EU reference, reset-enabled BG0's cleared map is transparent and all four outer edges are backdrop. The 31,763 backdrop pixels and 1,656 white BG2 pixels show why white RGB alone cannot determine a BLDCNT target. |
| Title | BG1, banks 0–2 | EU title's `0x0241` alpha blends BG0 rays over BG1; it does not independently pulse a BG1 source with no BG0 over it. JP uses a different native layer arrangement and is not certified here. |
| File selection | BG3, banks 11–14 | `0x0844` makes BG2 first target and BG3 second. Its local foreground blend is not a global BG3 fade. |
| Plain and titled pause backgrounds | BG3, bank 14 | Pause alpha `0x0e46` targets BG1/BG2 first and BG3 second. The heading OBJ is separate foreground. |
| Pieces and techniques references | BG2, bank 13 | During a tab transition `0x0e46` can target BG2 as the first alpha input, with BG3 among the second targets. The actual second input is required; treating all alpha as local/no-op is incorrect. |
| Fusion | BG1 banks 12–13, BG2 banks 11/13, BG3 bank 14 | Source ownership varies along the edges. The native success flash `0x3f90` brightens OBJ, not these BGs. Other source/second-layer combinations must use the actual current registers and window. |
| Fast travel | BG3, bank 5 | Group 93 supplies one fixed source regardless of health. Retained interaction traces include `0x3648`/`0x0f01`, where BG3 is a first alpha target and backdrop is an eligible second target, as well as `0x2f40` and neutral states. The actual second input and confirmation/scene-handoff publication remain their separate task. |
| Prologue page 1 | BG2, banks 11–14 | Native `0x2244` makes BG2 an alpha first target; BG1 and backdrop are eligible second targets. Resolve the actual second input and per-bank fade state. |
| Prologue pages 2–6 | BG2, banks 7–14 | The same `0x2244` policy applies; a single representative pixel does not describe the image. |
| Credits 1–8, ending card | BG2, banks 7–14 | Credits `0x2442` uses BG1 as alpha first target and BG2 as second target beneath text. A mask excluding bank 15 does not exclude these image banks. |
| Game Over, cleared credits, ending Save | Generated black field / backdrop, bank 0 | Game Over's observed `0xffff0001`, `0x0000ffff` and `0xffffffff` masks all include bank zero. Follow applicable backdrop effects; a screen made black by a fade is not evidence that an image is absent. |
| Standalone map hint, figurines | Live palette-zero backdrop, bank 0 | Map hint disables BG3 and copies entry 81 into zero. The figurine initializer clears BG3 and copies entry 251 into zero. Its prize flash uses `0x00bf` to brighten the backdrop; read the already-faded live field, skip palette fade and apply the relevant later effect once. |
| Local fusion map hint | Opaque BG3, palette index 95 / bank 5 | The filled `0x5001` tile is index 15 at every pixel for all three health preloads in the EU screen-6 recipe. Read live palette entry 95 and use BG3 effects. Equal unfaded palette-zero and entry-95 colors do not imply equal masked-fade behavior. |

The local-hint catalog source now records the BG3 entry-95 owner. Earlier neutral
field captures and hashes remain valid; they could not distinguish two equal
colors. `tools/background_effect_regression_test.py` runs the real staff-roll
mask command and `FadeMain` with `0xffff7fff`, and verifies bank 15 is excluded
while the illustration's banks 7–14 remain uniform. It also changes the mask
to exclude a used source bank and verifies the resolver returns MIXED, and
checks through the real `FadeVBlank` upload (using synthetic identity brightness
LUTs) that local-hint banks 0 and 5 can diverge. The actual retained credits
trace does not exercise the `0xffff7fff` state; it is not claimed as native
captured-pixel evidence for that branch.

`tools/background_effect_trace_audit.py` checks retained EU traces against the
decoded source layers. The title ray, pieces tab transition, fusion flash,
figurine prize, Game Over masks, fast-travel/prologue blend and credits text have distinct
observed alpha/brightness timelines. These traces do not resolve all fast-travel,
iris or regional intermediate effects.

`port/port_background_effect.{h,c}` supplies allocation-free, C/C++-compatible
helpers for explicit per-bank palette state and BLDCNT targets. Zero-intensity
states compare equal regardless of their unused color; mixed banks and missing
alpha input are reported, not silently replaced with a global fade. The RGB888
helpers retain eight-bit channels, composite straight alpha over black before
the palette fade, and clamp the native brightness and alpha coefficients. They
do not read game globals or implement the saved brightness LUT, source adapter,
margin compositor or scene-wide alpha/window policy. An already-faded live
palette field must bypass the palette-fade helper.

## HDMA classification and geometry

All current `SetVBlankDMA` call sites use incrementing sources. The classifier
in `port_hdma` identifies the destination footprint and source mode; a class is
not itself permission to extrapolate a table.

| Native producer / destination | Current policy |
| --- | --- |
| Steam overlay, light rays, Vaati appearance / BG3HOFS | Per-line scroll tables: native-height fallback until each continuation is established. |
| Rolling barrel / BG2PA–BG2Y | Eight-halfword affine records: native-height fallback. Repeating the last record is not mathematical affine continuation. |
| Detail map / BG3CNT | Per-line split table: native-height fallback until the split is defined for added rows. |
| Fade iris, light effects, ellipse helper and white triangle / WIN0H | Window tables: native-height fallback. The semantic effect and full-canvas shape must be selected explicitly; local windows must not affect unrelated margins. |
| Fixed BLDY source, fixed/reloaded destination, one halfword, windows disabled | The host allows reuse of line 159's brightness state for added rows. No current retail call site is asserted to use this form. Unknown modes, source/destination aliasing and mixed channels fail closed. |

The software PPU still executes the callback exactly 160 times and retains its
existing 160-row IO snapshot pool. For the verified fixed-brightness case, it
reuses the final snapshot for extra rows and the GPU-raster path falls back to
software for taller frames. All other active channels continue to cap the
effective height at 160. Hardware timing and VBlank behavior are unchanged.

`Port_Effect_BuildIrisRows` computes bounded, full-canvas circle spans using
the native midpoint algorithm and one translation of the 240×160 rectangle.
The regression links the game's actual `sub_0801E24C`/`sub_0801E290` code:
all radii 0–150, twelve centers and all five profile geometries agree inside
the native rectangle, including clipped edge centers. Added-row/column and
capacity cases are checked separately. The helper is not yet connected to a
scene-wide iris publisher or compositor, and it does not choose how the native
radius maps to the wider canvas. Triangle/ellipse and scene-effect policies
remain unimplemented; the HDMA/full-canvas parent task stays open.

## Verification and remaining work

Artifacts for the five languages are in `.tmp/background-effects-2026-10-27/`:
`eu-*-complete.json`, `complete-coverage-verification.json`,
`native-effects-prologue.json` and `ppu-parity-window.log`. Coverage compares
165 image recipes, native edge ownership, logo pixels and 30 local-hint/
figurine preload fixtures. Earlier runtime PNG captures/goldens are not replaced.

The staff-roll task ID is 4; the previous language-observation checker omitted
it while describing staff-roll coverage. The checker now includes it, and all
56 retained localized traces were revalidated, including 1,254 staff-roll
observations in each of the four post-credits interaction routes. Evidence is
in `staffroll-language-verification.json`; historical summary counts are not
retroactively changed.

EU source-target resolution is complete for the available fixture; it does not
claim intermediate screenshot acceptance for masked credits pixels or all
fast-travel transitions. Region-specific effect acceptance, scene-wide
windows/iris and full compositor publication remain separate tasks. The
compositor must consume the ownership data and publish effects before any
image-margin behavior is claimed. The full HDMA task also
remains open until the classified fallback cases and iris publication are
implemented and tested. Physical RG budgets and 4:3/32:9 enablement are
separate outstanding tasks.
