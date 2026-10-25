# Bodycam GAMMA Patch

Compatibility shaders, scripts and reticles for the [Bodycam engine](https://github.com/asuparabekon/xray-monolith-bodycam)
on G.A.M.M.A. and with 3D Shader Scopes (3DSS). The engine's own files run on plain Anomaly; the files
that patch other G.A.M.M.A. mods are in this mod.

You need it on G.A.M.M.A. and on any setup with 3DSS. Without it, 3DSS scopes use 3DSS's own shaders,
which have no PiP hooks, and the physical scope glass is off.

Requires **3D Shader Scopes for GAMMA 5.02**. Its see-through sights, night vision generations and gain,
fusion outlines, thermal battery and off screen, thermal reflex passthrough and Mark Switch dot offset all
keep working under PiP.

## Install (Mod Organizer 2)

1. Install the Bodycam engine: `bin\` into `Anomaly\bin`, `db\mods\00_modded_exes_gamedata.db0` into
   `Anomaly\db\mods`.
2. Install this zip as a mod named **Bodycam GAMMA Patch**.
3. Let it win file conflicts against: 3DSS for GAMMA 5.02, Mark Switch, Authentic Reticle for 3DSS,
   (01) EFT 3DSS And PRS Module, Screen Space Shaders 23, Beef's NVG, Enhanced Shaders (KennShade),
   FDDA Redone, GAMMA 3D PDA and Headlamp Animations, Boomsticks and Sharpsticks, and GAMMA script
   performance overrides.
4. Disable "3DSS PiP Compat Patch" and "Better Beef NVG PiP Compat Patch". This mod replaces both.
5. Clear `appdata\shaders_cache`.

If you already have a mod called **GAMMA Patch** (the old name), replace it instead of enabling both.

"PiP MAS Pizza Compatibility" (TheLostInPlace) works on top of this mod. If you use "3DSS Lens Hider
Hider", turn its lens hiding off: PiP draws on that lens.

## Contents

| Area | Files | Patches |
| --- | --- | --- |
| 3DSS lenses and reticles | `models_scope_reticle*`, `models_reflex_reticle*`, `scope_3dss_common.h`, `scope_common.h`, `scope_custom_*.h`, `scope_color_write.ps`, the `_pip_3dss_contract.ltx` | 3D Shader Scopes for GAMMA 5.02, Authentic Reticle for 3DSS, ARC and Pizza when enabled |
| Depth of field and scope glass | `depth_of_field.ps`, `models_scope_glass*`, `scope_glass_pbr_material.h`, `mod_system_scope_glass.ltx`, `textures/scope_glass/` | Screen Space Shaders 23, Enhanced Shaders (KennShade) |
| Night vision in the scope | `night_vision.h` | Beef's NVG |
| HUD animation scripts | `actor_effects.script`, `lam2.script`, `z_beefs_nvgs.script` | GAMMA 3D PDA and headlamp animations, FDDA Redone, Beef's NVG, GAMMA script performance overrides |
| Magnifier HUD | `bodycam_magnifier_hud.script` | Releases 3DSS's magnifier HUD override once its weapon is put away, without touching other owners |
| Boomsticks particles | `particletgz.ps` | Boomsticks and Sharpsticks |
| Core script copies | `callbacks_gameobject.script`, `_g_patches.script`, `options_*` and the rest of `CORE_MIRRORS` in the engine's `tools/release.py` | G.A.M.M.A. keeps older Monolith copies of these loose in `Anomaly\gamedata`, which win over the core package |
| Vector reticles | `reticle_atlas.h`, `reticle_atlas_reflex.h`, `reticle_vector.h`, the reticle textures, `z_3d_scopes_reticle_glow_mcm.script`, `z_3d_scopes_reticle_editor.script`, `ui_st_3d_scopes_reticle_glow.xml` | See below |
| PiP data for GAMMA weapons | `mod_pip_objective_offsets_gamma_patch.ltx`, `mod_pip_optic_physical_specs_gamma_patch.ltx` | Objective offsets and optic records for the FM Garand scopes |

[PROVENANCE.md](PROVENANCE.md) lists every file with its upstream and whether it was changed.

## Reticles

These sights get vector reticles drawn from SVG in place of their bitmaps:

- Razor (cortex): EBR-9 MOA
- March (shorty and cortex): DR-1F, in MIL. The ranging scale grows with zoom; the crosshair and fibre dot keep their size.
- 3DSS EOTech (eot, e0t2, exps3 and their magnifier combos): all six Mark Switch marks
- 1P59: the magnification dial turns with the zoom
- GIMP ACOG 2x: the Trijicon TA47 9.2 MOA triangle, lit by daylight through the fibre and by tritium at night

Each `.dds` ships next to its `.svg` and `.reticle.ini`, at the path of the bitmap it replaces. They work in
stock 3DSS and under PiP. The glow settings are on the Reticle Glow tab of the 3D Shader Scopes MCM menu.
The Reticle Editor (ImGui overlay, Mods > Reticle Editor) changes a held sight's reticle live and saves it
as your own copy in `appdata\reticles`; it can also load any SVG or grain image, and reset to the original.

To draw your own, see [docs/RETICLE_ATLASES.md](docs/RETICLE_ATLASES.md) and the engine's
`src/Layers/xrRender/reticle_vector/README.md`. The engine builds a reticle from its SVG when it loads the
texture. For other engines, its command-line tool writes the `.dds`:
`<engine>/tools/reticle_vector/svg_to_vector --all gamedata/textures gamedata/textures --check` rebuilds
this mod's atlases.

## Known issues

- With the old MSDF reticles the EOTech/G33 sometimes lost its ring or showed coloured fragments.
  Whether the vector reticles fix this is not yet checked.
- The reticle glow has not been checked on HDR10.

## Build

`python tools/package.py` writes `dist/Bodycam GAMMA Patch <version>.zip` with meta.ini, README.md,
PROVENANCE.md and `gamedata/` at the root. `--stage DIR` also writes the mod folder into a new or empty
DIR. The engine's `tools/release.py` and `tools/deploy.py` use this repo from a sibling checkout.

Pushes to `main` build a `release-package` artifact in GitHub Actions without publishing. To release,
bump the version in `meta.ini`, push, then push the matching tag (`v1.2.3`). The workflow checks the
archive against the source, then publishes it with `SHA256SUMS.txt`. Existing releases are never
overwritten.

## Credits

- 3D Shader Scopes: LVutner, party_50. 3DSS for GAMMA: Redotix99, Andtheherois, party50, MsPizza727.
- Mark Switch: party50 and meowie. Authentic Reticle for 3DSS: Napolemon.
- PiP and the PiP 3DSS, Better Beef NVG and grass patches: TheLostInPlace (m22).
- Screen Space Shaders: Ascii1457. Enhanced Shaders: KennShade. Beef's NVG: theRealBeef.
- FDDA Redone: lizzardman. Boomsticks and Sharpsticks: Mich and team. G.A.M.M.A.: Grok and the GAMMA team.
- Bodycam integration: asuparabekon.

The original shaders and scripts belong to their authors. This repository carries modified copies so they
work with the Bodycam engine.
