# MSDF reticles

Scope and holographic reticles drawn from a self-describing multi-channel distance-field atlas instead of a
bitmap: sharp at every zoom, true line weights when lines get thinner than a pixel, illumination that glows like
real emitted light (a screen-space glow that follows the eye's exposure and stays out of the game's bloom),
painted elements lit by the scene. The format and runtime integration are described below. Authoring is handled by [3DSS Reticle Studio](https://github.com/asuparabekon/3dss-reticle-studio). Shaders
and atlases are in this mod (`gamedata/`); the glow is a 3DSS feature of the Bodycam engine.

## Binding by texture path

An atlas ships at the path of the bitmap reticle it replaces and overrides it like any texture mod; the shaders
recognise it by its header. So it applies wherever that bitmap does: the sight's own model, 3DSS reticle
profiles, weapon sections with the sight built in, attachments, canted and reflex sights, and on an engine
without the hooks below. There is no script and no config: this replaces the `reticle_texture_msdf` profile key
and the profile script of design v3.

## Size

An atlas in scale mode `3dss` (the shipped GAMMA atlases) shows at the size the bitmap it replaces shows: the
shaders compute the coordinate the 3DSS path would sample that bitmap at (`scope_legacy_reticle_tcs` in PiP and
Reference, `reticle_tc` in the stock lens, `reflex_legacy_tc` in each reflex material) and map it through the
atlas's legacy span. The reticle then inherits that path's own parallax, focal-plane and zoom behaviour. Scale
mode `true` draws real angles registered with the image.

## Glow

The glow is a 3DSS engine feature (`r4_rendertarget_phase_3dss_reticle.cpp` in the Bodycam engine), built for
stock 3DSS first; PiP only carries its own images into it. While the engine holds its glow targets at outputs 2 and
3 (`C3DSSReticleGlow`), the 3DSS lens and reflex shaders write the lit core into `$user$reticle_emission` in scene
luminance: the Mark Switch colour (saturated, its magnitude or the atlas's brightness steps as the level) times
coverage times the atlas's `illumination_luminance` times `r__reticle_msdf_brightness`, and the same light weighted
by the atlas's glow strength into `$user$reticle_glow_source`. After bloom, the tone curve and the HDR10 bloom the
engine blurs the second in three Gaussian lobes (tight at full size, mid at half, wide at quarter) and scales the core
by the glow's exposure and the lobes by a power of it. The glow's exposure is the eye's: the adaptation luminance the
game's exposure (`$user$tonemap`) was made for, read back through the game's tone map settings and exposed again
with the glow's own floor (`r__reticle_msdf_glow_lowlum`) in place of `r2_tonemap_lowlum`, so the glow keeps
growing into the night where the game's clamp stops the scene. The core maps to the display with a hue-keeping soft
clip whose excess over white turns it white; the lobes keep their hue and approach `r__reticle_msdf_glow_halo`,
below white, so the reticle's lines still read inside a strong glow: a hot core with a coloured fringe, small by
day, large at night. The result covers the image behind it by `r__reticle_msdf_glow_tint` of its strength so the
glow stays coloured over a bright background. With HDR10 the frame is still before the display transform at that
point: the core goes in unclipped (white-shouldered) and the display transform tone maps it like the scene.

Lens dirt over a reticle (3DSS's dirt layer, stock and PiP lenses) dims its light and scatters part of it into the
glow: the core by the dirt's cover, the glow source by half of it.

The lobe sizes are one blur per frame, so they follow one sight: the atlas's `glow_size` of the sight drawn into
the glow targets that frame, the largest when two are drawn (a magnifier over a red dot, a canted dot beside a
scope), times `r__reticle_msdf_glow_size`. The engine reads it from the atlas file once per texture.

Stock 3DSS (PiP off): the reticle pass draws the reflex and holographic sights first (the engine routes the 3DSS
reflex meshes, `scopelense(10)`, into that pass instead of the HUD pass), then copies the frame and draws the 3DSS
lenses. A lens covers the glow behind it with its own: its reticle, and the reflex light its image shows from copies
of the glow targets taken with the frame, so a red dot seen through a magnifier glows magnified like the dot itself.

PiP and Reference: the lens composites own the reticle, so emission-only variants of their lens shaders draw the
light again into the glow targets. A red dot drawn into the eyepiece capture (a hybrid magnifier,
`r__svp_reflex_capture 1`) through the objective camera leaves its light in capture-space targets, and the PiP lens's
emission pass shows it where the lens shows the capture. The Reference image takes its capture during the combine,
before the red dot draws, so there the red dot itself goes into a capture-space target as well; the Reference image
pass writes the capture coordinate each lens pixel shows (`$user$svp_reference_capture_tc`), and one more pass adds
the red dot and its glow there through the image's pupil transmission. A reflex reticle's angle is measured from the
view's origin, so in the capture it is collimated for the objective camera and magnified with the image. With the
capture off the red dot is drawn over the lens in the main view, glow included.

Without the glow targets (an engine without this feature, exclusion or `r__reticle_msdf` off) the core goes into
the scene with a hue-keeping soft clip and no glow: the shaders read the engine's constants as zero and take that
path, so on a stock exe the reticle keeps its shape, marks, colour and a soft-clipped bright core.

## Holographic grain

Holographic laser grain is evaluated in reticle angle, shifted by eye position. It interpolates complex random
amplitudes and squares their magnitude, giving bright grains and dark gaps at unchanged mean light. The pixel
footprint averages the grains and suppresses unresolved contrast. This applies in the reflex material, including
stock 3DSS and magnifier captures; it does not change the reticle's scale or the 3DSS zoom mapping. The shipped
EOTech atlas authors stronger, larger grains and less temporal noise, with lower luminance so the white core
shoulder does not hide the grain.

The first three EOTech marks use clean circular rings and rounded cardinal ticks, with a 68 MOA ring centre line
and a centre dot authored 3 units wide to fit camera-visible proportions under the legacy mapping. The ring and
tick widths are authored at 3 MOA as an appearance fit; they are not claimed as published manufacturing dimensions.
The apparent dot diameter is an appearance fit, not a claim about the published 1 MOA dot. The two-dot spacing
and the other Mark Switch patterns are retained.
No bitmap glow or grain is included in these source paths. The atlas keeps its original legacy span, so changing
the source line weights does not change the installed 3DSS zoom mapping.

## Mark Switch and material states

Formats 5 (RGBA8) and 6 (BC5) separate pattern geometry from switch states. SVG `paint` layers retain
their fill as a non-emitting surface; `illum` layers provide emitted-light geometry and authored fill
colours. `ink` remains a legacy SVG alias for black paint. New illuminated shapes do not automatically
become black etching when off. Put a paint shape beneath them when an off-state surface is needed.

Each of up to 15 states references one pattern, a brightness (zero is off), and an optional colour override.
Without an override, each illum material retains its SVG colour. States can reuse one pattern without
copying atlas geometry. Pattern groups outside `Mark N` are shared. The selected state wraps by state
count; patterns remain independently numbered in tile records.

The DDS explicitly chooses authored colours or 3DSS colours. Authored mode ignores Mark Switch RGB
and level, while retaining its selection and dot offset. 3DSS mode uses its colour/level multiplied by
state brightness; black is off, with no authored-colour fallback. The existing bitmap and DDS v2-4 paths
are unchanged, including their old fallback and black unlit illumination behaviour.

Studio discovers patterns, creates initial states, exposes pattern/colour/brightness controls and previews
the selected state. All runtime data is in one DDS. This does not change the sight's 3DSS configuration:
that configuration must expose the intended number of Mark Switch selections.

## Pieces

The desktop authoring tool is [3DSS Reticle Studio](https://github.com/asuparabekon/3dss-reticle-studio). It supports SVG drop, appearance sliders, Mark Switch and a D3D11 preview running
the game's MSDF/reflex and engine glow shader sources. Geometry is compiled into a real atlas; appearance
changes update encoded descriptor headers while retaining payload blocks. The fixed clear-glass SDR scene
exposes preview/player inputs; pixel parity with a particular live game view has not been measured.
Sliders update continuously during dragging; SVG edits are watched and automatically recompiled.
Role layers `ink`, `illum`, and `paint` are required; fill colours no longer assign roles. Nested `mark N`
layers select patterns. The preview uses a rectangular crop and keeps one stable image during panning.
Eye adaptation follows the engine exposure-history equations with synthetic scene-average inputs; a
read-only user.ltx import matches player exposure/glow settings. Full live-scene parity remains unverified.
The GUI opens an SVG without a companion file. Appearance, Size, Colours and Export provide typed
controls for all authoring settings; no raw INI editor is shown. Uncalibrated drawings start at the visible,
editable default of 1 SVG unit per MOA. Save Project embeds the SVG and settings in a portable `.reticle`
file. Existing companion settings are still imported, and unknown keys/comments survive project saves.
DDS size estimates include mipmaps and compression/fallback layout bounds, then show actual compiled bytes.
Automatic compression falls back to RGBA8 when its compressed reference layout exceeds the atlas limit.
This allows SVG-only preview compilation; final exports still reopen the DDS and gate every mark before atomic replacement. See the [authoring guide](https://github.com/asuparabekon/3dss-reticle-studio/blob/main/AUTHORING.md) for the
workflow, preview limits and build instructions. This tool changes no 3DSS configuration or engine runtime.

| Where | What |
| --- | --- |
| [3DSS Reticle Studio](https://github.com/asuparabekon/3dss-reticle-studio) | SVG to atlas (`make_reticle.py`), the NumPy port of the shader and the offline gate (`gate_reticle.py`, once per mark). Build-time only, not part of the mod; uses the official msdfgen binary (MIT), downloaded on first use. |
| `shaders/r3/reticle_msdf.h` | Atlas decoding (per-mip header, descriptor, fail closed; formats 2 to 8), coverage, glow, paint. Evaluation only. |
| `models_scope_reticle(_precise).ps` | Stock 3DSS lens: angle from the frame pixel the lens shows (main camera FOV), marks in one slot under the tube shadow, the glow as outputs 2 and 3 with the reflex glow its image shows, or inline. |
| `scope_custom_reticle.h` | PiP Classic and Reference: angle from the published PiP image projection (or the frame mapping under fake PiP); paint lit by the image the lens shows; in the emission pass, the capture's reflex light. |
| `reticle_msdf_reflex.h`, `models_reflex_reticle*.ps` | Reflex and holographic sights: collimated angle from the viewpoint through the glass; holograms draw with their atlas's authored look (`ReticleHologram`: speckle, animated noise, softness, level, ghost, fringe, flicker). |
| `reticle_reflex_vertex.h` | Shared reflex pixel-input declaration, matching the 3DSS vertex shader's tangent/bitangent/normal outputs at TEXCOORD2/3/4. |
| `scripts/z_3d_scopes_reticle_glow_mcm.script` | The Reticle Glow tab of the 3D Shader Scopes MCM menu. |
| `textures/...` | The atlases, each at the path of the bitmap it replaces. |
| Bodycam engine core `reticle_glow.h`, `reticle_glow_{down,blur}.ps`, `reticle_emission_combine.ps` | The glow passes and the composite. |
| Bodycam engine core `*_emission.{s,ps}`, `reticle_emission_out.h` | PiP: emission-only variants of the PiP (also over deferred glass) and Reference lens shaders; they evaluate only the reticle. |
| Bodycam engine core `scope_reference_capture_light.{s,ps}` | PiP: a hybrid magnifier's red dot and its glow over the Reference image, at the capture coordinate `scope_reference_image.ps` writes. |

## Engine hooks (Bodycam engine)

| File | Hook |
| --- | --- |
| `xrRenderPC_R4/r4_rendertarget_phase_3dss_reticle.cpp` | the 3DSS reticle glow (it reads an atlas's glow size from the file, formats 3 to 8): its targets (`$user$reticle_emission`, `$user$reticle_glow_source`, their copies for the lenses, the lobes) and passes, created with the render target; `reticle_glow_begin`, `render_3DSS_reticles` (the stock reticle pass), `C3DSSReticleGlow`, `reticle_glow_note` (glow size), `reticle_glow_compose` |
| `xrRenderPC_R4/r4_3dss_reticle.cpp/.h` | the `r__reticle_msdf*` settings and the constants `reticle_msdf_params`, `reticle_msdf_control`, `reticle_holo_eye`, `reticle_msdf_reflex_glow` |
| `xrRender/xrRender_console.cpp`, `xrRender/Blender_Recorder_StandartBinding.cpp` | one call each registering the above |
| `xrRenderPC_R4/r4_rendertarget.cpp` | `reticle_glow_create()` in the main target's constructor |
| `xrRenderPC_R4/r4_rendertarget_phase_combine.cpp` | `reticle_glow_begin()` before `render_forward()`; `reticle_glow_compose()` after combine_2, the HDR10 bloom and lens flare |
| `xrRender/r__dsgraph_build.cpp`, `r__dsgraph_types.h`, `r__dsgraph_render.cpp`, `r__dsgraph_manager.h` | `scopelense(10)` HUD meshes go to `mapScopeReflexHUD` while 3DSS is on (`r__3Dfakescope`); `r_dsgraph_render_ReflexSorted` draws them |
| `xrRenderPC_R4/svp_optics.cpp` (PiP) | `phase_3DSSReticle` runs `render_3DSS_reticles` when PiP takes no part; `draw_reflex` and the hybrid magnifier hold the glow targets with `C3DSSReticleGlow`; `draw_scope` publishes `svp_reticle_projection` and keeps a lens texture loaded; one emission draw after each PiP or Reference lens reticle; the emission shaders build with the lens glue in `EnsureScopeShaders` |
| `xrRenderPC_R4/r4_R_render.cpp` (PiP) | `CRender::Render` builds the lens glue (`EnsureScopeShaders`) on the level's first frame instead of the first aim |
| `xrRenderPC_R4/r4_shader_warmup.cpp/.h` | the shader warm-up: while the level loads, worker threads compile the pixel shaders of the 3DSS sight materials and, with PiP, of the lens glue and the Reference passes from sources read ahead on the loader thread; nothing else is touched off the render thread |
| `xrRenderPC_R4/r4_loader.cpp`, `xrRenderPC_R4/r4.cpp` | `shader_warmup_start` at level load, `shader_warmup_stop` at unload; `shader_compile` builds its defines and target with `shader_compile_options` / `shader_compile_target` (shared with the warm-up) and takes the warm-up's bytecode on a cache miss |
| `xrRenderPC_R4/svp_reticle_emission.cpp` (PiP) | `draw_reticle_emission`, `reticle_capture_begin` (capture-space targets of a hybrid reflex), `draw_reference_capture_light` (a hybrid reflex over the Reference image), `reticle_msdf_image` (PiP reticle paint sees the eyepiece capture) |

## Settings

Engine settings, also in MCM under 3D Shader Scopes > Reticle Glow (glow size, 0.25 to 1.5, and strength scale
their lobe vectors as a whole). `r__reticle_msdf` 1 draws atlases with their distance field and glow; 0 draws them from their
coverage alone with the legacy colouring and no glow target (A/B). `r__reticle_msdf_bloom_exclusion` 1 keeps the
glow out of bloom. The player's settings act on top of each atlas's authored values:

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
| `r__reticle_msdf_speckle` | 1 | Holographic speckle | scale on each holographic atlas's authored speckle and animated noise depth |
| `r__reticle_msdf_line_weight` | 0 | Thin line weight | 0 = true darkness of thin lines, 1 = bold |
| `r__reticle_msdf_debug` | 0 | | 1 tints a lens whose atlas fails validation |

## Atlas format

Formats 5/6 use the existing RGBA8/BC5 payloads with a 168-byte global descriptor. Bytes 0-39 retain
the old layout; bytes 40-47 hold state count, colour source and padding. Bytes 48-167 hold fifteen
8-byte state slots: pattern, override flag, RGB, brightness and padding. The per-tile records remain
24 bytes. A minimum 64-header-texel reserved cell keeps the state table available after mip stripping.
Old readers fail closed on these new versions. Glow-size lookup in the engine accepts both versions.

Formats 7/8 extend the standalone Studio workflow with named material groups. Groups under paint and
illum become separately editable materials. A state references a pattern and stores a colour override
or SVG-fill selection for each material; illum also stores brightness/off. Shared groups are available
in every pattern. Colour-only changes never duplicate geometry. SVG IDs provide stable project keys;
labels provide display names. The existing 16-tile capacity remains, with at most 16 materials and
15 states. Multiple fills or detail tiles can consume more than one tile per material.

The global descriptor is 1144 bytes: byte 42 is material count; bytes 168-183 map tiles to materials;
bytes 184-1143 store 15 x 16 RGBA entries. RGB is a colour override when alpha bit 7 is set, otherwise
the tile keeps its SVG fill. Alpha's low seven bits store illumination brightness / 127. Header t5.w
holds the high byte of descriptor texel count; a 128-header-texel reserved cell supports resident mip 2.
Formats 7/8 bound each material's coverage taps to its own tile with a zero border, including trilinear
filtering, so small groups cannot sample neighbouring tiles or headers. Old versions keep their sampler.
Each state slot's former padding bytes (offsets 6 and 7) hold a little-endian 16-bit hidden-material
mask. A set bit hides that material's paint or illumination, independent of its stored colour and
brightness. Existing zero-filled slots keep all materials visible. Visibility requires the updated
shader decoder; earlier format-7/8 shaders ignore this mask. Geometry is unchanged.
Both the runtime shader and engine glow-size lookup accept 7/8. The legacy state defaults are resolved
into the material table by the converter and are not multiplied again at runtime.

The descriptions below document the retained legacy storage formats:


Format 4, BC5 DDS (DX10 header), used by the shipped atlases: one byte per texel. A tile
holds either one true signed distance in R and the exact area coverage in G, or, where one distance would round
its sharpest corners under strong magnification, the multi-channel distance split over two squares side by side
(R, G in its own, B and the coverage in the one to its right). The multi-channel median does not survive block
compression of the three channels together, but it does channel by channel; the distance texels near the edge are
fitted most closely, the coverage keeps exact zeros and each block's sum. The tool picks per tile
(`msdf-reticle-studio/gate_reticle.py`, `choose_kinds`): one distance where the tile's sharpest corners, magnified 4
and 8 times its texel size, keep their shape within 0.02 soft IoU of the multi-channel tile, multi-channel otherwise
or where that fits the same texture; it keeps the whole texture uncompressed when the compressed one misses the
gate's corners check that the uncompressed one meets. The header and descriptor are stored exactly: each header
texel is one 4x4 block whose endpoint bytes are its four bytes (R0, G0 on the block's top row, R1, G1 below), so
the header grid is a quarter of the mip in each direction and its reserved cell is 128 texels (256 for more than 7
tiles). Tiles never touch across the atlas edge, where the wrapping sampler would mix them on coarse mips.

Format 3, RGBA8 UNORM DDS (`compression = none`; format 2 still reads, with true angles and default
illumination). Per tile: RGB = multi-channel signed distance regenerated for every mip with one tile-normalised
range, A = exact area coverage of that mip's texels.

In both, every mip that can be the resident top mip (texture quality strips 0 to 2) carries a header in row 0 and
the descriptor from row 1 of a reserved cell (32 header texels at mip 0); `msdf-reticle-studio/reticle_format.py`
documents the bytes. Format 3 added the legacy span, the illumination luminance, the glow strength, the glow size
and the brightness steps; format 4 reads the same descriptor. A recognised atlas that fails any check draws no
marks.

| atlas | format 3 (RGBA8) | format 4 (BC5) |
| --- | --- | --- |
| EBR-9 (`cortex.dds`) | 2048 x 1024, 10.7 MB | 4096 x 1024, 5.3 MB: every tile multi-channel (the fine ink tile's corners need it, the others fit the same texture) |
| EOTech (`wpn_addon_scope_eotech_crosshair_dot.dds`) | 1024 x 1024, 5.3 MB | 2048 x 1024, 2.7 MB: the bracket mark needs multi-channel corners; all other tiles use the same packing |
