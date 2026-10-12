// PIP_COMPAT_PATCH scope_custom_lens 20261007 3dss502
/*
	=====================================================================
	Addon      : Shader 3D Scopes
	Link       : https://www.moddb.com/mods/stalker-anomaly/addons/shader-3d-scopes
	Authors    : LVutner, party_50
	PiP Author : m22

	God-Queen who fixed everything: MsPizza727

	All credit to original authors.
	=====================================================================
*/

#include "scope_3dss_common.h"
#include "svp_hooks_lens.h"
#include "svp_lens_optics.h"

#ifndef SVP_LENS_PUPIL_LOSS
#define SVP_LENS_PUPIL_LOSS(world) (1.0 - svp_exit_pupil_transmission(svp_scope_lens_tc(world)))
#endif

float4 scope_custom_lens(v_out I, Scope s) {
	if (svp_lens_flat_window_cull())
		return float4(0, 0, 0, 0);

	float DIRT_INTENSITY = decoded_dirt_intensity();
	float lum = current_lum() * 2;
	SvpLensProfile profile = svp_authored_lens_profile();

	float3x3 TBNw_inv = float3x3(I.w_T, I.w_B, I.w_N);
	TBNw_inv = transpose(TBNw_inv);
	float3 normalmap = sample_lens_normalmap(s.tc0, 3);
	float3 lensnormal = normalize(float3(dot(normalmap, TBNw_inv[0]), dot(normalmap, TBNw_inv[1]), dot(normalmap, TBNw_inv[2])));
	float4 dirt = s_dirt.Sample(smp_base, s.tc0);
	dirt = saturate(float4((1 - dirt.rgb) * 1.5 * calc_model_lq_lighting(lensnormal), dirt.a * DIRT_INTENSITY));

	bool is_flat = RETICLE_TYPE == RT_FLAT_SCREEN;
	const bool true_pip = shader_scope_params.w < -1.5;
	float3 view_direction = normalize(eye_position - s.w_P);
	float3 surface_normal = dot(lensnormal, view_direction) < 0.0 ? -lensnormal : lensnormal;
	float surface_cosine = saturate(abs(dot(surface_normal, view_direction)));
	float3 layer1_color = svp_optic_path_coating(svp_front_reflection_path(),
		surface_cosine);
	float3 layer2_color = svp_optic_path_coating(svp_middle_reflection_path(),
		surface_cosine);
	float3 layer3_color = svp_optic_path_coating(svp_deep_reflection_path(),
		surface_cosine);
	float2 lens_position = float2(dot(s.w_P - svp_lens_center.xyz, svp_lens_right.xyz),
		dot(s.w_P - svp_lens_center.xyz, svp_lens_up.xyz)) * svp_lens_center.w * 2.0;
	float2 screen_position = s.hpos.xy * output_res.zw;
	float pupil_loss = true_pip
		? SVP_LENS_PUPIL_LOSS(s.w_P)
		: 1.0;
	float ads_blend = saturate(zoomRotateFactor());
	float aligned_visibility = pupil_loss * svp_lens_ads_reflection_visibility();
	float reflection_visibility = true_pip
		? lerp(1.0, aligned_visibility, ads_blend)
		: 1.0;
	float source_visibility = true_pip ? reflection_visibility : 1.0;
	SvpLensChannels channels = svp_evaluate_lens(lens_position, screen_position, s.w_P,
		svp_lens_right.xyz, svp_lens_up.xyz, surface_normal, view_direction,
		profile, is_flat, reflection_visibility, source_visibility);
	float4 reflections = svp_composite_lens(channels);

	// Specular
	float3 specular = sample_specular(s.tc0.xy, 0, 3, SPECULAR_FACTOR, dirt.a, TBNw_inv, s.w_P, s.w_N, SETTING(SETTINGS, ST_SEE_THROUGH)) * layer1_color;
	specular += sample_specular(s.tc0.xy, 100, 2, SPECULAR_FACTOR * 0.1, dirt.a, TBNw_inv, s.w_P, s.w_N, SETTING(SETTINGS, ST_SEE_THROUGH)) * layer2_color;
	specular += sample_specular(s.tc0.xy, 40, -1, SPECULAR_FACTOR * 0.1, dirt.a, TBNw_inv, s.w_P, s.w_N, SETTING(SETTINGS, ST_SEE_THROUGH)) * layer3_color;
	if (true_pip)
	{
		specular *= reflection_visibility;
		dirt.a *= reflection_visibility;
	}

	// The legacy lens vignette is an authored cosmetic mask. Under true PiP it
	// duplicates the physical tube/pupil boundary and appears as a fixed dark
	// annulus once the lens reaches the eye during ADS.
	float4 vignette = true_pip ? float4(0, 0, 0, 0)
		: float4(0, 0, 0, smoothstep(0.4, 2, 2 * length(s.tc0.xy - float2(0.5, 0.5))));

	float4 final_scope_2 = float4(0,0,0,0);

	final_scope_2 = rgba_blend(final_scope_2, vignette);
	final_scope_2 = rgba_blend(final_scope_2, reflections);

	float4 final_scope_3 = float4(0,0,0,0);

	final_scope_3 = rgba_blend(final_scope_3, dirt);

	final_scope_2.rgb = tonemap(final_scope_2.rgb, s.v_P);
	final_scope_3.rgb = tonemap(final_scope_3.rgb, s.v_P);

	float4 final_scope = rgba_blend(final_scope_2, final_scope_3);
	float4 specular_white = float4(1,1,1,specular.r * 0.35);
	final_scope = rgba_blend(final_scope, specular_white);
	final_scope.rgb += specular * 0.25;

	// Keep aperture ownership in authored mesh UV space. Reflection and ghost
	// coordinates are intentionally free to move inside this fixed boundary.
	float2 aperture_position = (s.tc0 - s.center) / max(s.radius, 0.0001);
	float objective_aperture = is_flat ? 1.0
		: svp_optic_objective_aperture(aperture_position);
	final_scope.a *= objective_aperture;
	return final_scope;
}
