# Object overrides

Object overrides replace or decorate a fixed-screen game object with a PNG
while keeping the object's normal logic, position, slide timing and draw order.
They are the object-level counterpart to the background artwork in
`assets/override/<profile>/`.

## Folder and names

```
assets/override/objects/<name>.png
assets/override/objects/<name>_<language>.png
```

- `<name>` is a stable, human-readable object name. It contains no IDs or
  frame numbers.
- The language-specific file is preferred. If it is absent, the generic
  `<name>.png` is used. If neither exists, the game draws the original object.
- Language suffixes are `japanese`, `english`, `french`, `german`, `spanish`
  and `italian`. EU language slots map to the same names as the direct
  USA/JP language enum.
- After adding or editing a PNG, use **F8 → Display → Reload object artwork**
  to refresh the cache without restarting.

## The file-select heading is split into frame + text

The game draws the heading ribbon and the localized text as one OAM frame. The
text is baked into the same tiles as the blue ribbon, so a full-frame PNG
would force one language's text onto every language.

The shipped asset instead separates them at the composite level:

```
assets/override/objects/file_select_header.png
```

This is a shared 164x24 ribbon frame with a transparent window over the text
area. It is composited **over** the original heading; the original localized
text shows through the window. One PNG therefore serves EU
English/French/German/Spanish/Italian, USA English and JP Japanese, and the
game continues to draw the correct language.

Using the extracted file unmodified renders byte-identical pixels in every
supported language. Adding new artwork above the frame (for example the cut
top border) appears in every language without extracting or maintaining
per-language copies.

The opaque ribbon pixels outside the text window are language-independent; the
text window is the only part that differs between languages. The heading hook
therefore requests only the generic `file_select_header.png`; language
variants are useful for other objects, not for this shared frame.

Full builds stage missing files from `assets/override/objects/` into
`dist/<region>/assets/override/objects/`, so the shared frame ships by
default. A file already present in the dist folder wins, which keeps local
artist edits across rebuilds. Repository/dev builds also fall back to the
source `assets/override/objects/` path when the runtime asset tree does not
contain the file.

## Size and placement

The PNG may be the same size as the original object or larger. The runtime
aligns the **centre of the PNG** with the **centre of the original object's
on-screen bounds**. Transparent padding is allowed and is the normal way to
extend artwork.

For example, the file-select heading is 164x24 and its centre is 82,12 from
its top-left. To add 8 pixels of new artwork above it:

1. Create a 164x40 canvas.
2. Place the existing 164x24 frame at `x=0, y=8` (so its centre stays at the
   new canvas centre, 82,20).
3. Draw the new top border in rows 0..7.
4. Leave rows 32..39 transparent (or use them for extra bottom artwork).

The extra top rows then appear above the original heading without moving the
original pixels. The text window stays aligned with the game's text.

## Regenerating the extracted heading

The room-capture harness can dump the PPU state:

```bash
TMC_AUTOPLAY=1 TMC_ROOMCAP=1 TMC_ROOMCAP_FILESELECT=1 \
TMC_ROOMCAP_DUMP=/tmp/fsel_ppu TMC_ROOMCAP_OUT=/tmp/fsel \
TMC_BASEROM=/path/to/baserom.gba SDL_VIDEODRIVER=dummy \
SDL_AUDIODRIVER=dummy ./tmc_pc --no-audio
```

Then extract the shared frame from `ppu_a.bin`, passing the same run's
`fsel_a.png` as the reference:

```bash
python3 tools/export_object_override.py \
  --dump /tmp/fsel_ppu_a.bin \
  --reference /tmp/fsel_a.png \
  --profile file_select_header \
  --output assets/override/objects/file_select_header.png
```

The tool renders the heading's OAM pieces to get its transparency mask, tries
both OBJ VRAM bases (0x10000/0x14000), picks the one that matches the
reference, and then uses the reference frame's exact RGB for opaque pixels.
For this profile it also punches the text window transparent (32,0..135,15),
leaving a shared ribbon frame. `--keep-text` produces a full-frame PNG
instead; `--list-pieces` prints the selected OAM entries; `--char-base` can
force a base if needed.

Any language's capture works for the shared frame; the text is removed by the
window.

## Validation

```bash
python3 tools/validate_object_overrides.py
```

Checks that the shipped shared frame exists and has the expected dimensions.
Runtime compositing is also covered by the byte-identical captures for EU
English/French/German/Spanish/Italian, USA English and JP Japanese.

## Adding another object

At the object's draw site:

1. Measure the original frame bounds with `Port_Draw_MeasureFrame()`, or pass
   fixed native-screen bounds if the object has a stable rectangle.
2. Call `Port_ObjectOverride_Submit()` with the logical name, language,
   bounds and the OBJ palette bank used by the original (for fade matching).
3. If it returns true, skip the original sprite for that frame (for example
   set `super->spriteSettings.draw = 0`).
4. Keep all other logic (movement, timers, sound, collision) untouched.

Use `Port_ObjectOverride_SubmitOverlay()` instead when the PNG has transparent
areas that must reveal the original object (as the heading does); leave the
original sprite enabled in that case. Both APIs share the same centred
placement and fade handling.

The API is declared in `port/port_object_override.h`. Requests are cleared
each frame by `FlushSprites()`, and the PNG is composited onto the final
native/tall canvas before the LCD/colour post-process. Coordinates are native
240x160 screen pixels; the compositor adds the tall-canvas margin offset.

The implementation only changes pixels. It does not change collision, input,
animation state or object lifetime.

## Limits

- One request per logical name per frame. Submitting the same name again just
  updates that request.
- Overrides are intended for fixed-screen objects whose bounds are in native
  screen coordinates. Live-world objects would need their scroll subtraction
  done by the caller.
- The PNG follows the original OBJ palette's fade table. The GBA brightness
  setting (0/1/2) is not currently baked into PNG colours; the extracted
  frame uses the neutral palette.
- The shared heading frame is only used for the main `CHOOSE A FILE` frame.
  The new-file, copy and erase headings keep their original localized
  artwork.
- The text window is fixed at `(32,0)..(135,15)` for the heading. If a future
  layout moves the text, update the profile's `text_window`.
