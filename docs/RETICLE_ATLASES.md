# Reticle atlases

Scope and holographic reticles in this mod are vector drawings, not bitmaps. Each texture holds the
reticle's shapes as quadratic curves (atlas format 12), and the shader computes how much of each shape
covers every pixel. Edges stay sharp at any zoom, and lines thinner than a pixel keep their real darkness.

A reticle follows its sight's focal plane, and a dual focal plane reticle can also have parts that always
keep their size. Lit parts glow like emitted light, with a screen-space glow that follows the eye's
exposure and stays out of the game's bloom. Painted parts are lit by the scene. An atlas is a few to a
few hundred KB. The shaders and atlases are in this mod's `gamedata/`, each atlas next to its `.svg` and
`.reticle.ini`. The glow itself is a 3DSS feature of the Bodycam engine.

Older atlases (MSDF formats 2 to 8, vector formats 9 to 11) are recognised but not drawn. With
`r__reticle_msdf_debug 1` they show magenta, and the engine logs each one once. Rebuild them from their SVG.

## Binding by texture path

An atlas ships at the path of the bitmap it replaces and overrides it like any texture mod. The shaders
tell it apart from a bitmap by its header. It therefore applies wherever the bitmap did: the sight's own
model, 3DSS reticle profiles, weapons with a built-in sight, attachments, canted and reflex sights, and
engines without the hooks listed below. No script or config is involved.

## Making one

A step-by-step guide is in [RETICLE_AUTHORING.md](RETICLE_AUTHORING.md).

Draw the reticle as filled shapes in an SVG with named Inkscape layers (`paint`, `illum`, `glow`,
`light`, `mark N`, `sfp`, `dial`, `center`) and write its `.reticle.ini`. The supported SVG features, every setting
and the build commands are in the Bodycam engine's `src/Layers/xrRender/reticle_vector/README.md`.
Then either:

- put `<texture>.svg` and `<texture>.reticle.ini` at the texture's path, in `appdata/reticles/` (your own
  reticle; the Reticle Editor saves there) or in `gamedata/textures`. The engine builds the atlas when it
  loads the texture. In `gamedata/textures` a `.dds` at the same path wins unless `r__reticle_svg 1`.
- or build the `.dds` with the engine's command-line tool.
  `<engine>/tools/reticle_vector/svg_to_vector --all gamedata/textures gamedata/textures --check` rebuilds
  every atlas in this mod from the drawings next to them and checks each against its drawing.

The format is documented in `gamedata/shaders/r3/reticle_vector.h`.

## Editing in game

Hold a sight and open Mods > Reticle Editor in the ImGui overlay. Pick the reticle profile (or, for a reflex
sight, its reticle material) and change the settings; the sight updates as you go. Under the hood the editor
(`z_3d_scopes_reticle_editor.script`) writes the settings to `appdata/reticle_editor_session.reticle.ini` and
the engine rebuilds the texture from its `.svg` (`r__reticle_edit`).

Your own copy of a sight's reticle lives in `appdata/reticles/<texture>.svg` and `.reticle.ini`, and the
engine builds it ahead of everything else.

- **Save** writes the settings there, with copies of the shipped drawing and grain image, and reloads the
  texture (`r__reticle_reload`).
- **Load SVG** puts any drawing there at once, so a sight with a bitmap reticle can get a vector one.
- **Load image** copies a grain PNG next to it.
- **Reset to original** deletes your copy; the shipped reticle (or bitmap) comes back.
- **Revert** returns to the settings as loaded or last saved.
- **Your drawing** shows the path of your copy's SVG, empty while the original shows.
- **Hold aim** keeps the sight raised while the window is open.
- **Inspect** draws the HUD closer (zoom 1x to 8x, the engine's `r__reticle_inspect`, never saved) so the
  reticle can be seen large while you adjust it.

Everything is saved under `appdata/reticles`; installed mod files are never changed. The drawing itself is
edited in Inkscape. (Both loads browse the real folders with the engine's `bodycam.list_dir`, because
rescanning the game's file system under MO2 would drop the mods' files from its index.)

## Settings

Engine settings, also in MCM under 3D Shader Scopes > Reticle Glow. There, glow size (0.25 to 1.5) and
strength scale all three lobes together. The names keep the old `r__reticle_msdf_` prefix so saved values
carry over. `r__reticle_msdf_bloom_exclusion 1` keeps the
glow out of bloom. These settings act on top of each atlas's own values:

| setting | default | MCM | meaning |
| --- | --- | --- | --- |
| `r__reticle_msdf_brightness` | 1 | Brightness | scale on the atlas's illumination luminance |
| `r__reticle_msdf_glow_size` | 3, 12, 40 | Glow size | Gaussian sigma of the tight, mid and wide lobe, pixels at 1080p, times the drawn atlas's `glow_size` |
| `r__reticle_msdf_glow_weight` | 1, 0.35, 0.15 | Glow strength | weight of each lobe against the core (0 = off) |
| `r__reticle_msdf_glow_exposure` | 1 | Night bloom | power of the glow's exposure on the lobes: 1 scales them like the core, more keeps them small by day and large at night |
| `r__reticle_msdf_glow_lowlum` | 0.2 | Night adaptation | the glow's exposure floor in place of `r2_tonemap_lowlum`: lower lets the glow grow further into the night than the game's tone map lets the scene |
| `r__reticle_msdf_glow_halo` | 0.55 | Glow ceiling | the display level the lobes approach, below white, so the lines read inside the glow |
| `r__reticle_msdf_glow_shoulder` | 1 | White-hot core | how fast light above display white turns the core white |
| `r__reticle_msdf_glow_tint` | 0.5 | Glow tint | how much the glow covers the image behind it instead of adding to it |
| `r__reticle_msdf_holo_level` | 1 | Holographic brightness | scale on each holographic atlas's authored level |
| `r__reticle_msdf_speckle` | 1 | Holographic grain | scale on each holographic atlas's mask depth |
| `r__reticle_msdf_line_weight` | 0 | Thin line weight | 0 keeps thin lines at their partial pixel coverage, 1 draws them bold |
| `r__reticle_msdf_debug` | 0 | | 1 tints a lens whose atlas is older or fails validation |
| `r__reticle_svg` | 0 | | 1 builds `<texture>.svg` ahead of `<texture>.dds` (authoring) |
| `r__reticle_edit` | | | `<texture> <settings file>`: rebuild a loaded reticle from its `.svg` with these settings, live (Reticle Editor) |
| `r__reticle_reload` | | | `<texture>`: load a loaded reticle again from its sources, live (after the Reticle Editor saved or reset it) |
| `r__reticle_edit_status` | | | the last edit's or reload's result: `pending`, `ok: ...` or `error: ...` (read only, not saved) |

## Size and focal planes

Every path computes once where a pixel lands on the reticle, for both focal planes (`reticle_atlas_view`).

With a `legacy_span` (the shipped GAMMA atlases), the atlas shows at the size of the bitmap it replaces.
The shaders compute the coordinates the 3DSS path would sample that bitmap at (`scope_legacy_reticle_tcs`
in PiP and Reference, `reticle_tc` and `reticle_lens_tc` in the stock lens, `reflex_legacy_tc` in each
reflex material) and map them through the span, so the atlas inherits that path's parallax. Parts that grow
with the zoom follow `reticle_tc`, scaled as the sight's 3DSS focal plane says. Parts in the `sfp` layer
follow the lens coordinate, the way the 3DSS dual focal plane bitmaps (reticle type 9) sample their
fixed-size part, so the two match at the lowest zoom.

Without a span the atlas draws real angles registered with the image, and `sfp` parts scale with the
sight's magnification over the atlas's `calibrated_magnification`. A reflex sight does not zoom, so both
planes sit at the same angle. A reticle with parts in one plane is drawn once; with both, twice, each at
its own place.

Parts in the `dial` layer keep their size like `sfp` parts and also turn about the centre with the zoom,
as 3DSS turns the dial of its 1P59 bitmap (reticle type 5, the blue channel). All paths use one angle,
`reticle_dial_angle` in `reticle_atlas.h`, from the zoom fraction each path already computes for the 1P59:
`scope_dial_angle` in the stock lens, `scope_dial_fraction` in PiP and Reference (which maps true PiP to
the measured dial angles). The dial is a third pass over the grid at the `sfp` place, turned back by that
angle. A reflex sight's dial does not turn.

## Glow

The glow is a 3DSS feature of the engine (`r4_rendertarget_phase_3dss_reticle.cpp`), built for stock 3DSS;
PiP only feeds its own images into it.

While the engine holds its glow targets at outputs 2 and 3 (`C3DSSReticleGlow`), the 3DSS lens and reflex
shaders write the lit core into `$user$reticle_emission`, in scene luminance. The core is the Mark Switch
colour (saturated, with its magnitude or the atlas's brightness steps as the level) times coverage, times
the atlas's `luminance`, times `r__reticle_msdf_brightness`. The same light, weighted by the atlas's glow
strength, goes into `$user$reticle_glow_source`.

After bloom, tone mapping and the HDR10 bloom, the engine blurs the glow source into three Gaussian lobes at
full, half and quarter resolution. It scales the core by the glow's exposure and the lobes by a power of it.
The glow's exposure follows the game's eye adaptation (`$user$tonemap`), but with its own floor,
`r__reticle_msdf_glow_lowlum`, in place of `r2_tonemap_lowlum`. A lower floor lets the glow keep brightening
in the dark after the scene's exposure has hit its limit.

The core maps to the display through a hue-keeping soft clip, where light above white turns it white. The
lobes keep their hue and approach `r__reticle_msdf_glow_halo`, below white, so the reticle's lines still
read inside a strong glow. The result is a hot core with a coloured fringe, small by day and large at night.
It covers the image behind it by `r__reticle_msdf_glow_tint` of its strength, so the glow stays coloured
over a bright background. On HDR10 the frame has not been through the display transform yet at that point,
so the core goes in unclipped and the display transform tone maps it with the scene.

Lens dirt over a reticle (3DSS's dirt layer, stock and PiP lenses) dims its light and scatters part of it
into the glow: the core by the dirt's cover, the glow source by half of it.

The lobe size is one blur per frame, so it follows one sight: the `glow_size` of the atlas drawn into the
glow targets that frame (the largest when two are drawn, such as a magnifier over a red dot or a canted dot
beside a scope), times `r__reticle_msdf_glow_size`. The engine reads it from the atlas once per texture, or
from the settings for one it builds from SVG.

**Stock 3DSS (PiP off).** The reticle pass draws the reflex and holographic sights first (the engine routes
3DSS reflex meshes, `scopelense(10)`, into that pass instead of the HUD pass), then copies the frame and draws
the 3DSS lenses. A lens covers the glow behind it with its own: its reticle, plus the reflex light its image
shows, taken from copies of the glow targets. A red dot seen through a magnifier therefore glows magnified,
like the dot.

**PiP and Reference.** The lens composites own the reticle, so emission-only versions of their lens shaders
draw the light again into the glow targets. A red dot drawn into the eyepiece capture (a hybrid magnifier,
`r__svp_reflex_capture 1`) leaves its light in capture-space targets, and the PiP lens's emission pass shows
it where the lens shows the capture. The Reference image takes its capture during the combine, before the
red dot is drawn, so there the red dot also goes into a capture-space target. The Reference image pass
writes the capture coordinate each lens pixel shows (`$user$svp_reference_capture_tc`), and one more pass
adds the red dot and its glow there through the image's pupil transmission. A reflex reticle's angle is
measured from the view's origin, so in the capture it is collimated for the objective camera and magnified
with the image. With the capture off, the red dot is drawn over the lens in the main view, glow included.

Without the glow targets (an engine without this feature, or `r__reticle_msdf_bloom_exclusion 0`), the core
goes into the scene through the hue-keeping soft clip, with no glow. The shaders read the engine's
constants as zero and take that path, so on a stock exe the reticle keeps its shape, marks, colour and a
soft-clipped bright core.

## Holographic look

A holographic sight (a `[holo]` section) draws softer and dimmer than an LED reticle, with an optional ghost
image, colour fringe and low-step flicker. Its grain is the author's mask image (`[mask]`). The grain is
fixed in reticle angle and moves with the eye's position relative to the sight, like laser speckle. It is
relative to its mean, so the average light is kept, and grain finer than a screen pixel fades out. With
`animate` it cross-fades to a new random place that many times a second; by default it is still. The shipped
EOTech uses a laser-speckle mask with 1.8 MOA grains.

## Mark Switch and colours

The Mark Switch selection picks the pattern: shapes in a `mark N` (or `mark 1, 3`) layer show with those
marks, everything else with every mark. Layers:

- `paint`: keeps its fill as a surface lit by the scene; etched lines are black paint.
- `illum`: etched, and lights up.
- `glow`: paint that also lights up.
- `light`: only emits, unseen while the illumination is off (a fibre dot).

The light's colour is the sight's Mark Switch colour (black is off; `[illumination] colour = sight`, the
default) or each lit shape's own fill (`colour = drawing`, for sights without Mark Switch colours such as the
3DSS EOTech). `brightness_steps` sets each mark's level.

A sight lit by daylight and tritium instead of a battery (`[illumination] fibre` and `tritium`, like the
3DSS ACOG, reticle type 3) is always lit, whatever the Mark Switch says. Its lit core is `fibre` times the
sky light's luminance (`L_ambient + L_hemi_color + L_sun_color`, the weather's light on an upward face, the
same sum the lens optics use) plus the constant `tritium`. By day the fibre makes it bright against the
scene; at night the tritium glow is what shows. The hue is the drawing's with `colour = drawing`, otherwise
the Mark Switch colour, or white for a sight without one.

## Files in this mod

| Where | What |
| --- | --- |
| `shaders/r3/reticle_atlas.h` | The settings record, the pixel's place per focal plane (`reticle_atlas_view`), illumination, emission, glow outputs, paint; recognises older atlases. Evaluation only. |
| `shaders/r3/reticle_vector.h` | The format 12 layout, its checks, the per-pixel coverage of the curves per focal plane, the holographic mask. |
| `models_scope_reticle(_precise).ps` | Stock 3DSS lens: angle from the frame pixel the lens shows (main camera FOV), marks in one slot under the tube shadow, the glow as outputs 2 and 3 with the reflex glow its image shows, or inline. |
| `scope_custom_reticle.h` | PiP Classic and Reference: angle from the published PiP image projection (or the frame mapping under fake PiP); paint lit by the image the lens shows; in the emission pass, the capture's reflex light. |
| `reticle_atlas_reflex.h`, `models_reflex_reticle*.ps` | Reflex and holographic sights: collimated angle from the viewpoint through the glass; holograms draw with their atlas's authored look (`ReticleHologram`). |
| `reticle_reflex_vertex.h` | Shared reflex pixel-input declaration, matching the 3DSS vertex shader's tangent/bitangent/normal outputs at TEXCOORD2/3/4. |
| `scripts/z_3d_scopes_reticle_glow_mcm.script` | The Reticle Glow tab of the 3D Shader Scopes MCM menu. |
| `textures/...` | The atlases, each at the path of the bitmap it replaces, with their SVG and settings beside them. |
| `scripts/z_3d_scopes_reticle_editor.script` | The in-game Reticle Editor (ImGui). |

The engine side (glow targets and passes, the 3DSS reticle pass, the SVG builder hook) is listed in the
Bodycam engine's `src/Layers/xrRender/reticle_vector/README.md`.
