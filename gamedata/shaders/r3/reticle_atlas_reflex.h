// Reticle atlases on reflex and holographic sights (models_reflex_reticle*.ps). The reticle is collimated: its
// angle is the direction from the viewpoint (the view's origin, also a PiP objective that sees the sight) through
// the glass, measured against the glass normal, so it sits at infinity like the bitmap path; in 3dss scale mode it
// follows the bitmap's own coordinate instead. A reflex sight does not zoom, so both focal planes sit at that
// angle. Reflex output is light only (additive blend), no ink. While the engine holds the reticle glow targets at
// slots 2 and 3 (reticle_atlas_reflex_glow.x) the light goes there, out of bloom.
//
// Holographic sights draw with the look their atlas authors (ReticleAtlas.holo_*): softer and dimmer, the author's
// grain mask fixed in reticle angle and moving with the eye's position relative to the sight
// (reticle_vector_grain), and optionally a ghost image, colour fringes and low-step flicker.
// r__reticle_msdf_holo_level and r__reticle_msdf_speckle scale the level and the grain depth on top.
#ifndef RETICLE_ATLAS_REFLEX_H
#define RETICLE_ATLAS_REFLEX_H

#include "reticle_atlas.h"

#ifndef RETICLE_HOLO_EYE_DECLARED
#define RETICLE_HOLO_EYE_DECLARED
uniform float4 reticle_holo_eye; // engine: the physical eye in the current view space, w = 1 when set
#endif

// A reflex material's outputs: the scene (SV_Target0) and the reticle glow core and glow source (SV_Target2, 3).
// With the reflex blend an output with alpha 0 leaves its target as it was.
struct ReflexReticleOut
{
	float4 colour : SV_Target0;
	float4 glow : SV_Target2;
	float4 glow_source : SV_Target3;
};

// A reflex material's outputs: the bitmap, or the atlas's light into the glow target while it is bound, else into
// the scene. The material blends by source alpha onto what is there.
ReflexReticleOut reticle_atlas_reflex_out(ReticleAtlas atlas, float4 bitmap, float3 light)
{
	ReflexReticleOut o;
	o.colour = bitmap;
	o.glow = 0;
	o.glow_source = 0;
	if (atlas.recognised)
	{
		const bool glow = reticle_atlas_reflex_glow.x > 0.5 && reticle_atlas_emission_external();
		if (reticle_atlas_reflex_glow.x < -0.5)
			light = 0;
		o.colour = glow ? 0 : float4(reticle_atlas_inline(light), 1);
		o.glow = glow ? reticle_atlas_core_out(light) : 0;
		o.glow_source = glow ? reticle_atlas_glow_out(atlas, light) : 0;
	}
	return o;
}

// view-space position, tangent frame and UV of the glass fragment; dot_offset = the mark switch zero offset
// in bitmap UV (0.16 rad per unit, the legacy reticle scale); legacy_tc = where the material's bitmap path would
// sample (in the Mark Switch cell), for 3dss scale mode. Returns the light in scene luminance.
float3 reticle_atlas_reflex(Texture2D t, ReticleAtlas atlas, float3 P, float3 T, float3 B, float3 N, float2 tc0,
	float2 dot_offset, float2 legacy_tc)
{
	const float3 eye = reticle_holo_eye.w > 0.5 ? reticle_holo_eye.xyz : float3(0, 0, 0);
	const float3 n = normalize(N);
	const float3 tt = normalize(T - n * dot(T, n));
	const float3 bb = normalize(B - n * dot(B, n));
	const float depth = max(abs(dot(P, n)), 1e-5);
	const float units = ratlas_units_per_radian(atlas);
	const float2 angle = float2(dot(P, tt), dot(P, bb)) / depth * units + dot_offset * 0.16 * units;
	const ReticleView view = reticle_atlas_view(atlas, angle, 1.0, legacy_tc, legacy_tc);
	const bool holo = (atlas.flags & RATLAS_FLAG_HOLO) != 0 && atlas.valid;
	ratlas_soften = holo ? atlas.holo_softness : 1.0;

	// the sight's centre from the glass UV gradient, for the eye position relative to the sight
	const float2 tc_dx = ddx(tc0), tc_dy = ddy(tc0);
	const float3 p_dx = ddx(P), p_dy = ddy(P);
	const float det = tc_dx.x * tc_dy.y - tc_dy.x * tc_dx.y;
	const float inv = abs(det) > 1e-12 ? 1.0 / det : 0.0;
	const float3 dp_du = (p_dx * tc_dy.y - p_dy * tc_dx.y) * inv;
	const float3 dp_dv = (p_dy * tc_dx.x - p_dx * tc_dy.x) * inv;
	const float3 centre = P + dp_du * (0.5 - tc0.x) + dp_dv * (0.5 - tc0.y);
	const float2 eye_mm = float2(dot(eye - centre, tt), dot(eye - centre, bb)) * 1000.0;

	if (!atlas.valid)
		return (atlas.recognised && reticle_atlas_control.y > 0.5) ? float3(0.25, 0, 0.25) : 0;

	// black is off; a sight with no Mark Switch colours needs an atlas with its own mark colours
	const float4 illumination = reticle_atlas_lighting(atlas, markswitch_color, markswitch_current.x);
	// The marks at up to four reticle angles: the image (k 0), a hologram's red and blue colour fringes, the red
	// and blue images a little apart (1, 2), and its ghost image from an internal reflection (3). One loop, so the
	// marks are compiled once instead of four times. Red and blue images under a quarter pixel apart are not split.
	const float2 fringe_shift = float2(0.5 * atlas.holo_fringe, 0.0);
	const bool fringe = holo && atlas.holo_fringe > 0.25 * length(view.ffp_dx);
	const bool ghost = holo && atlas.holo_ghost_level > 0.0;
	float3 light = 0;
	[loop] for (uint k = 0; k < 4; ++k)
	{
		if ((k == 1 || k == 2) && !fringe)
			continue;
		if (k == 3 && !ghost)
			continue;
		const float2 offset = k == 0 ? 0.0 : (k == 1 ? fringe_shift : (k == 2 ? -fringe_shift : -atlas.holo_ghost_offset));
		const float3 e = reticle_atlas_emission(atlas,
			reticle_atlas_marks(t, atlas, reticle_atlas_view_shift(view, offset), markswitch_current.x), illumination);
		if (k == 0)
			light = e;
		else if (k == 1)
			light.r = e.r;
		else if (k == 2)
			light.b = e.b;
		else
			light += atlas.holo_ghost_level * e;
	}
	if (holo)
	{
		const bool set = reticle_atlas_control.w > 0.5;
		const float level = atlas.holo_level * (set ? max(reticle_atlas_params.x, 0.0) : 1.0);
		float grain = reticle_vector_grain(t, atlas, view.ffp, eye_mm, view.ffp_dx, view.ffp_dy,
			set ? max(reticle_atlas_params.z, 0.0) : 1.0);
		// pulse-width dimming shows as flicker at the lowest steps only
		if (atlas.holo_flicker > 0.0)
			grain *= 1.0 - atlas.holo_flicker * saturate(1.0 - illumination.w / 0.35) * step(0.55, frac(timers.x * 9.7));
		light *= level * grain;
	}
	return light;
}

#endif
