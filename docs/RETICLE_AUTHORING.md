# Making a reticle

This guide takes you from a blank drawing to a reticle in game. You need Inkscape, a text editor, and the Bodycam engine v0.3.1 or newer with this mod.

A reticle is two files with the same name as the texture they replace:

- `<texture>.svg`: the drawing.
- `<texture>.reticle.ini`: its size and lighting.

The engine turns them into a reticle when the game loads that texture. You don't need any build tool.

## 1. Find the texture

A reticle replaces one texture, the bitmap the sight already uses, and takes over wherever that bitmap
shows. The shipped ones show the pattern:

| Sight | Texture |
| --- | --- |
| EOTech (all 3DSS EOTech variants) | `wpn/wpn_addon_scope_eotech_crosshair_dot` |
| 1P59 Giperon | `wpn/scope_reticles/1p59` |
| ACOG 2x | `wpn/scope_reticles/acog_2x` |
| Razor HD Gen III, EBR-9 | `wpn/scope_reticles/cortex/ebr9/cortex` |
| March DR-1F | `wpn/scope_reticles/cortex/dr_1f/cortex` |

For another sight, use the path of its reticle `.dds` under `gamedata/textures`, without the extension.

## 2. Draw it

Set the drawing's units to something you can measure in. The shipped reticles mostly use 1 SVG unit =
1 MOA or 1 mil, with the centre at 0,0 (for example `viewBox="-80 -80 160 160"`).

Draw every visible part as a **filled shape**, and put it in a layer named after its role:

| Layer | What it draws |
| --- | --- |
| `paint` | A surface lit by the scene, in its fill colour. Etched lines are black paint. |
| `illum` | Etched lines that light up. They are black while the illumination is off. |
| `glow` | Paint that also lights up. |
| `light` | Light only, invisible while the illumination is off (a fibre dot). |

By default the whole reticle follows the sight's focal plane as its 3DSS profile sets it: it grows with the
zoom on a first-focal-plane sight and keeps its size on a second-focal-plane one. (A reticle without
`legacy_span`, step 3, always grows with the zoom.)

Three more layers change how the shapes inside them behave. They nest with the role layers in either order:

| Layer | Effect |
| --- | --- |
| `sfp` | Always keeps its size when the sight zooms. Only needed for a reticle with parts in both focal planes, like the DR-1F's fixed crosshair over its growing ranging scale. |
| `dial` | Keeps its size and turns with the zoom, like the 1P59's magnification dial. |
| `mark 1`, `mark 2, 3`, … | Shows only with those Mark Switch marks (1 to 15). Shapes outside any `mark` layer show with every mark. |

An optional `center` layer holds one shape whose middle is the bore centre. It isn't drawn. Without it,
the centre is the middle of the viewBox.

**Rules the engine enforces:**

- Fills only, solid and opaque. Turn strokes into shapes with **Path > Stroke to Path**.
- No gradients, masks, clip paths, filters, stylesheets or even-odd fills.
- Holes are fine. For overlapping pieces, **Path > Union** gives the cleanest outline.
- A piece lying wholly inside another piece of the same path draws nothing, and the log warns about it.

Save as **Inkscape SVG**. Plain SVG drops the layer names, and the engine needs them. Another editor works
if it writes Inkscape's layer attributes (`inkscape:groupmode="layer"` and `inkscape:label`).

## 3. Write the settings

Create `<texture>.reticle.ini` next to the drawing. The smallest one says how big a drawing unit is:

```ini
[reticle]
units = moa      ; or mil
scale = 1        ; drawing units per MOA (or mil)
```

If you're replacing a bitmap reticle and want yours to show at the same size, add `legacy_span`: how many
MOA (or mil) the old bitmap spans edge to edge. Measure a known mark in the bitmap: for a 1024 px bitmap
with 3.261 px per MOA, `legacy_span = 1024 / 3.261 = 314.03`. All the shipped GAMMA reticles do this, and
their `.reticle.ini` files show the working.

### Lighting

Battery-lit reticles light in the sight's Mark Switch colour by default:

```ini
[illumination]
luminance = 8         ; how bright the lit core is
glow_strength = 1     ; how strong the glow is against the core
glow_size = 1         ; how far the glow spreads
```

- `colour = drawing` lights each shape in its own fill colour. Use it for sights without Mark Switch
  colours, like the EOTech.
- `brightness_steps = 0.2, 0.4, 0.7, 1` gives each Mark Switch mark its own brightness.
- An always-on fibre and tritium sight, like the ACOG, uses `fibre` (brightness from daylight) and
  `tritium` (the constant night glow) instead of `luminance`:

  ```ini
  [illumination]
  fibre = 1
  tritium = 0.03
  ```

### Holographic sights

A `[holo]` section draws the reticle softer and dimmer, like an EOTech. Add a `[mask]` with a grey PNG for
laser grain. The shipped EOTech is the reference: `wpn/wpn_addon_scope_eotech_crosshair_dot.reticle.ini`
and its `_speckle.png`.

Every key, with its range and default, is listed in the engine's
[reticle format reference](https://github.com/asuparabekon/xray-monolith-bodycam/blob/bodycam-mt/src/Layers/xrRender/reticle_vector/README.md).
Any key not in that list is an error.

## 4. Try it in game

Put both files in `Anomaly/appdata/reticles/`, using the texture's path. For example:

```
appdata/reticles/wpn/scope_reticles/acog_2x.svg
appdata/reticles/wpn/scope_reticles/acog_2x.reticle.ini
```

Your copy there always wins over the shipped one, and no installed files are changed. Then:

1. Hold the sight and open **Mods > Reticle Editor** in the ImGui overlay.
2. Change the settings and watch the sight update. **Inspect** brings the HUD closer so you can see
   detail, and **Hold aim** keeps the sight raised.
3. **Save** keeps your settings in `appdata/reticles`.
4. After editing the SVG in Inkscape, run `r__reticle_reload <texture>` in the console to see the change.

**Load SVG** in the editor puts any drawing on the held sight in one step, so a sight that only has a
bitmap reticle can get a vector one. **Reset to original** deletes your copy.

## 5. When it doesn't show

Search the log for `RETICLE-SVG`:

- `* [RETICLE-SVG] <texture>: N shapes in M pieces` means it built.
- `! [RETICLE-SVG] ...` lists what's wrong: an unsupported SVG feature, a shape outside any role layer, an
  unknown settings key. The texture falls back to the old bitmap until it's fixed.

`r__reticle_edit_status` shows the result of the last edit or reload.

## 6. Shipping it in a mod

Put the `.svg` and `.reticle.ini` (and any grain PNG) in your mod's `gamedata/textures`, at the texture's
path. The engine builds them when the game loads the texture.

A `.dds` at the same path wins over the drawing. To ship a prebuilt one instead, build it with the engine's
`tools/reticle_vector/svg_to_vector`; `--check` compares the result with the drawing.

## See also

- [RETICLE_ATLASES.md](RETICLE_ATLASES.md): how reticles look and work, the glow, the console settings and
  the Reticle Editor in detail.
- The shipped reticles in `gamedata/textures`: every `.reticle.ini` explains where its numbers come from.
