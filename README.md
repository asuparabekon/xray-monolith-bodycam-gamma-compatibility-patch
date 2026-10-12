# Bodycam GAMMA Patch

The GAMMA-specific gamedata for the [Bodycam X-Ray engine](https://github.com/asuparabekon) (a Monolith fork that carries the PiP true picture-in-picture scopes). The engine and its core gamedata work on plain Anomaly. This mod adds everything that patches other G.A.M.M.A. mods.

**Required on G.A.M.M.A.** (and on any setup with 3D Shader Scopes). Without it, 3DSS scopes render through 3DSS's stock shaders, which lack the PiP hooks, and the physical scope glass is off.

Targets **3D Shader Scopes for GAMMA 5.02**. 3DSS 5.02's own files cover the reflex lens, scope back/zwrite, `mark_adjust.h` and the scope vertex shader, so this mod does not replace them. The reflex reticle blenders are replaced only to add the sampler MSDF reticles need. The 5.02 features work under PiP: transparent sights (ADS with both eyes open) keep their see-through housing around the PiP lens, NV generations and gain, fusion outlines (all four colours), the thermal battery and powered-off screen, the thermal reflex passthrough and the Mark Switch dot offset.

## What it contains

| Area | Files | Patches |
| --- | --- | --- |
| 3DSS reticles and lenses | `models_scope_reticle{,_precise}.{ps,s}`, `models_reflex_reticle{,_3db,_simple,_simple_3db}.ps`, `scope_3dss_common.h`, `scope_common.h`, `scope_custom_{image,lens,reticle,shadow}.h`, `scope_color_write.ps`, `mod_system_zzzz…_pip_3dss_contract.ltx` | 3D Shader Scopes for GAMMA 5.02, Authentic Reticle for 3DSS, ARC/Pizza when enabled |
| SSS 23 depth of field and the physical scope glass material | `depth_of_field.ps`, `models_scope_glass*`, `scope_glass_pbr_material.h`, `mod_system_scope_glass.ltx`, `textures/scope_glass/*` | Screen Space Shaders 23, Enhanced Shaders (KennShade) |
| NVG objective view | `night_vision.h` | Beef's NVG (Better Beef NVG PiP compat, plus the bodycam depth focus) |
| HUD animation scripts | `actor_effects.script`, `lam2.script`, `z_beefs_nvgs.script` | GAMMA 3D PDA/headlamp animations, FDDA Redone, Beef's NVG, GAMMA script performance overrides (each is that file plus the bodycam HUD motion calls) |
| Magnifier HUD ownership | `bodycam_magnifier_hud.script` | Releases 3DSS's global magnifier HUD override when its weapon is no longer active, including same-slot replacements. Keeps an open HUD editor and other override owners intact. |
| Boomsticks particles | `particletgz.ps` | Boomsticks and Sharpsticks (adds the Reference scope particle hook) |
| Core script mirrors | `callbacks_gameobject.script`, `_g_patches.script`, `options_*` and the other files listed in the Bodycam repo's `tools/release.py` `CORE_MIRRORS` | Exact copies of the Bodycam core files. A G.A.M.M.A. install keeps older Monolith copies loose in `Anomaly\gamedata`, which outrank the core `.db0`. |
| MSDF reticles | `reticle_msdf.h`, `reticle_msdf_reflex.h`, `models_reflex_reticle*.s`, `textures/wpn/scope_reticles/cortex/ebr9/cortex.dds`, `textures/wpn/wpn_addon_scope_eotech_crosshair_dot.dds`, `z_3d_scopes_reticle_glow_mcm.script`, `ui_st_3d_scopes_reticle_glow.xml` | The EBR-9 MOA of the Razor (cortex) and the six Mark Switch marks of the 3DSS EOTech (eot, e0t2, exps3 and their magnifier combos) as self-describing distance-field atlases at the paths of the bitmaps they replace; the shaders recognise them by their header (see `docs/MSDF_RETICLES.md`). Their glow is the Bodycam engine's 3DSS reticle glow, in stock 3DSS and under PiP; its settings are the Reticle Glow tab of the 3D Shader Scopes MCM menu. `r__reticle_msdf 0` draws them from coverage alone for comparison |
| PiP data for GAMMA weapons | `mod_pip_objective_offsets_gamma_patch.ltx`, `mod_pip_optic_physical_specs_gamma_patch.ltx` | PiP's authored objective offsets and optic mapping for the FM Garand scopes (DLTX into the core tables) |

[PROVENANCE.md](PROVENANCE.md) lists every file with its upstream and whether it is identical or modified.

## Install (Mod Organizer 2)

1. Install the Bodycam engine release (`bin\` into `Anomaly\bin`, `db\mods\00_modded_exes_gamedata.db0` into `Anomaly\db\mods`).
2. Install this zip as a mod named **Bodycam GAMMA Patch**.
3. Give it a **higher priority** than: 3DSS for GAMMA (5.02), Mark Switch, Authentic Reticle for 3DSS, (01) EFT 3DSS And PRS Module, Screen Space Shaders 23, Beef's NVG, Enhanced Shaders (KennShade), FDDA Redone, GAMMA 3D PDA and Headlamp Animations, Boomsticks and Sharpsticks, and GAMMA script performance overrides. In MO2, ensure this patch wins the file conflicts; left-pane position depends on the selected sort order.
4. Disable the older "3DSS PiP Compat Patch" and "Better Beef NVG PiP Compat Patch" mods: this mod replaces both.
5. Clear `appdata\shaders_cache` after installing or updating.

Optional: "PiP MAS Pizza Compatibility" (by TheLostInPlace) works on top of this mod; its contract asks for `pip_3dss_compat_patch`, which this mod still provides. If you use "3DSS Lens Hider Hider", turn its lens hiding off: PiP draws on the lens it would hide.

If updating an existing **GAMMA Patch** installation, replace or rename that mod rather than enabling both copies.

## Build

`python tools/package.py` writes `dist/Bodycam GAMMA Patch <version>.zip` (meta.ini, README.md, PROVENANCE.md and gamedata at the root, sorted with a fixed timestamp). `--stage DIR` also writes the mod folder into a new or empty DIR outside the checkout. The Bodycam repo's `tools/release.py` and `tools/deploy.py` call it through a sibling checkout (`..\xray-monolith-bodycam-gamma-compatibility-patch`).

## Reticle authoring

[3DSS Reticle Studio](https://github.com/asuparabekon/3dss-reticle-studio) is the separate desktop editor. Open a layered SVG, edit named paint/illumination groups and Mark Switch states, then export a DDS. Only that exported texture is installed in the game.

The [authoring guide](https://github.com/asuparabekon/3dss-reticle-studio/blob/main/AUTHORING.md) covers SVG preparation, calibration, materials, preview controls and export. The [runtime reference](docs/MSDF_RETICLES.md) explains how this patch reads the atlas and integrates it with 3DSS. Studio source and binaries are maintained in their own repository.

## Credits

- **3D Shader Scopes**: LVutner, party_50; 3DSS for GAMMA: Redotix99, Andtheherois, party50; MsPizza727.
- **Mark Switch**: party50 and meowie. **Authentic Reticle for 3DSS**: Napolemon.
- **PiP (true picture-in-picture scopes)** and the PiP 3DSS / Better Beef NVG / grass compatibility patches: TheLostInPlace (m22).
- **Screen Space Shaders**: Ascii1457. **Enhanced Shaders**: KennShade. **Beef's NVG**: theRealBeef.
- **FDDA Redone**: lizzardman. **Boomsticks and Sharpsticks**: Mich and team. **G.A.M.M.A.**: Grok and the GAMMA team.
- Bodycam integration: asuparabekon.

All credit for the original shaders and scripts goes to their authors. This repository redistributes modified copies so they work with the Bodycam engine.

## Compatibility notes

MSDF rendering works with PiP disabled. Existing bitmap reticles retain their rendering path. The engine supplies the exposure-aware glow passes; this patch supplies compatible lens/reticle shaders and atlases. See [MSDF reticles](docs/MSDF_RETICLES.md) for material states and settings.

An intermittent EOTech/G33 rendering issue has been observed: a missing ring with coloured fragments in the lens. Its cause remains unresolved. HDR10 glow has not received visual validation.

## Automated builds and releases

Pushes to `main` build and validate a downloadable `release-package` artifact in GitHub Actions. They do not publish a release. The workflow can also be run manually for a build.

To publish, update `meta.ini`, push the source, then push a matching version tag such as `v1.2.3`. Tags must match the source version. The workflow builds the ZIP, checks its contents, uploads it with `SHA256SUMS.txt`, and publishes the release only after the build succeeds. Use a new version for each release; existing releases are not overwritten.

The mod archive is checked against its source files and keeps gamedata/ at the root for MO2.
