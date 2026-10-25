// PIP_COMPAT_PATCH scope_custom_reticle 20261007 3dss502
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
#include "svp_hooks_reticle.h"
#include "svp_hooks_shadow.h"
#include "svp_hooks_inside.h"
#include "reticle_atlas.h"

#ifndef SVP_RETICLE_PROJECTION_DECLARED
#define SVP_RETICLE_PROJECTION_DECLARED
// engine, per lens draw: projection of the PiP image (1/_11, 1/_22, _31, _32), zero when there is none
uniform float4 svp_reticle_projection;
#endif

// ARC 3DSS dual-focal reticles use indices 10 and 11.
// Relocate the unused mark magnifier to 12 so all three styles coexist.
#undef RT_MARK_MAGNIFIER
#define RT_MARK_MAGNIFIER 12
#define RT_DFP_ALT 10
#define RT_DFP_ALT2 11

float dial_angle_fraction(float mag)
{
	const float mags[8] = {3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
	const float fractions[8] = {0.0000, 0.2032, 0.3907, 0.5557, 0.6956, 0.8155, 0.9151, 1.0000};

	mag = clamp(mag, mags[0], mags[7]);
	for (int i = 0; i < 7; ++i)
	{
		if (mag <= mags[i + 1])
		{
			const float t = (mag - mags[i]) / (mags[i + 1] - mags[i]);
			return lerp(fractions[i], fractions[i + 1], t);
		}
	}

	return fractions[7];
}

#ifdef SCOPE_REFERENCE_MARK_ONLY
#define SCOPE_REFERENCE_OCULAR_DECLARED
Texture2D scope_reference_ocular;
uniform float4 scope_reference_transfer; // x = the Reference image's tone map input gamma
#endif

// The light behind an atlas reticle pixel, in the domain the lens image was written in: painted marks reflect it.
// Reference: the optical image tone mapped as its image pass does. PiP: the eyepiece capture at the image
// pass's coordinate with its exposure terms. Fake PiP: the main frame pixel the lens shows.
float3 scope_reticle_background(Scope S, float2 image_tc)
{
#ifdef SCOPE_REFERENCE_MARK_ONLY
	const float3 raw = scope_reference_ocular.Load(int3(S.hpos.xy, 0)).rgb;
	half4 low, high;
	tonemap(low, high, pow(max(raw, 0), 1.0 / max(scope_reference_transfer.x, 0.01)), s_tonemap.Load(int3(0, 0, 0)).x);
	return low.rgb;
#else
	float3 back = s_image.SampleLevel(smp_base, image_tc, 0).rgb * LENS_COLOR;
	if (shader_scope_params.w < -1.5 && svp_exposure.x > 0)
	{
		const float lm = s_tonemap.Load(int3(0, 0, 0)).x;
		const float ls = s_tonemap_svp.Load(int3(0, 0, 0)).x;
		if (lm > 0.0001 && ls > 0.0001)
			back *= clamp(svp_exposure.x * ls / lm, 0.25, 4.0);
	}
	if (shader_scope_params.w < -1.5 && svp_exposure.y > 0)
		back *= svp_exposure.y;
	return back;
#endif
}

#if defined(RETICLE_ATLAS_EMISSION_PASS) && !defined(SCOPE_REFERENCE_MARK_ONLY)
// engine (PiP): a reflex drawn into the eyepiece capture (a hybrid magnifier) leaves its atlas reticle light in these
// capture-space targets; x 1 = they hold this frame's light
Texture2D s_reticle_capture_emission;
Texture2D s_reticle_capture_glow;
uniform float4 reticle_capture_glow;

// That light where the lens shows the capture (the image pass's coordinate), through the lens's transmission and
// its dirt, added to the emission pass's core and glow source
void scope_reticle_capture_light(Scope S, float transmission, float dirt, inout float4 core)
{
	if (reticle_capture_glow.x < 0.5)
		return;
	const float shown = (SETTING(SETTINGS, ST_SEE_THROUGH) ? 1.0 : zoomRotateFactor()) * transmission;
	float2 image_tc = S.sfp;
	svp_img_flat_window(image_tc, S);
	svp_img_fix_flipped_lens(image_tc, S);
	const float3 light = s_reticle_capture_emission.SampleLevel(smp_rtlinear, image_tc, 0).rgb * LENS_COLOR * shown;
	const float3 glow = s_reticle_capture_glow.SampleLevel(smp_rtlinear, image_tc, 0).rgb * LENS_COLOR * shown;
	core = float4(core.rgb + light * (1.0 - dirt), 1.0);
	reticle_atlas_glow_source = float4(reticle_atlas_glow_source.rgb + glow * (1.0 - 0.5 * dirt), 1.0);
}

// The lens's dirt over this pixel, as the lens pass lays it over the image
float scope_reticle_dirt(Scope S)
{
	return saturate(s_dirt.Sample(smp_base, S.tc0).a * decoded_dirt_intensity());
}
#endif

// Atlas reticle under PiP and Reference: the lens UV is the PiP image UV, so the published image projection
// gives the world angle of every lens pixel and the reticle stays registered with the image. dirt: the lens dirt
// over the pixel, which dims the reticle's light and scatters part of it into the glow (emission pass).
float4 scope_atlas_reticle(float3 background, float transmission, float dirt, ReticleAtlas atlas, ReticleView view)
{
	if (!atlas.valid)
	{
	#ifdef RETICLE_ATLAS_EMISSION_PASS
		return 0;
	#else
		return reticle_atlas_control.y > 0.5 ? float4(1, 0, 1, 0.25) : 0;
	#endif
	}
	const float4 illumination = reticle_atlas_lighting(atlas, markswitch_color, markswitch_current.x);
	const ReticleMarks marks = reticle_atlas_marks(s_reticle, atlas, view, markswitch_current.x);
	// 3DSS 5.02 thermal battery: the reticle is part of the display, it dims with it and goes out when flat
	float power = 1.0;
	if (scope_battery_powered())
		power = SCOPE_BATTERY_CHARGE <= THERMAL_CRITICAL_CHARGE ? 0.0 : thermal_power_signal(SCOPE_BATTERY_CHARGE);
	const float shown = SETTING(SETTINGS, ST_SEE_THROUGH) ? 1.0 : zoomRotateFactor();
	const float3 light = reticle_atlas_emission(atlas, marks, illumination) * (transmission * shown * power);
	#ifdef RETICLE_ATLAS_EMISSION_PASS
	// into the glow targets, or straight into the scene (the engine draws this pass there) without them
	if (!reticle_atlas_emission_external())
		return float4(reticle_atlas_inline(light * (1.0 - dirt)), 0);
	reticle_atlas_glow_source = reticle_atlas_glow_out(atlas, light * (1.0 - 0.5 * dirt));
	return reticle_atlas_core_out(light * (1.0 - dirt));
	#else
	// paint is lit by the light the lens gathers, the image behind the reticle
	const float3 paint = reticle_atlas_paint(marks, dot(background, float3(0.299, 0.587, 0.114)));
	float alpha = marks.paint;
	float3 colour = paint;
	alpha *= transmission * (power > 0.0 ? 1.0 : 0.0);
	// an engine without the emission pass (reticle_atlas_control unset) gets the lit core here, without glow
	if (reticle_atlas_control.w < 0.5)
	{
		const float3 lit = reticle_atlas_inline(light);
		const float cover = max(alpha, max(lit.r, max(lit.g, lit.b)));
		colour = cover > 0.0 ? (colour * alpha + lit) / cover : 0;
		alpha = cover;
	}
	return float4(colour, alpha);
	#endif
}

// Without a PiP image (fake PiP) the lens shows the main frame: the reticle angle comes from the frame
// pixel each lens pixel shows (the SCOPECOORD_TO_TEXCOORD image mapping), the bore from the lens centre
// at zero parallax.
float2 scope_reticle_frame_angle(Scope S, ReticleAtlas atlas, out float2 frame_tc)
{
	float image_project = 0.8 * zoomFactor() - 0.8;
	float image_size = 0.2 * zoomFactor() + 0.8;
	if (min_zoom_1x())
	{
		image_project *= zoomPercent();
		image_size = 1 + (image_size - 1) * zoomPercent();
	}
	const float zoom = lerp(1, image_size, zoomRotateFactor());
	const float shift = lerp(0, image_project, zoomRotateFactor());
	const float3x3 TBN = cotangent_frame(S.v_N + 0.0039, S.v_P.xyz + 0.0039, S.tc0.xy);
	float3 view = normalize(float3(dot(-S.v_P.xyz, TBN[0]), dot(-S.v_P.xyz, TBN[1]), dot(-S.v_P.xyz, TBN[2])));
	view.x *= output_res.y / output_res.x;
	const float2 screen_tc = S.hpos.xy * output_res.zw;
	frame_tc = (screen_tc - 0.5) / zoom + 0.5 + view.xy * svp_effective_mas(mas_scale()) * shift;
	const float2 tc_dx = ddx(S.tc0), tc_dy = ddy(S.tc0);
	const float det = tc_dx.x * tc_dy.y - tc_dy.x * tc_dx.y;
	const float2 to_centre = 0.5 - S.tc0;
	const float2 centre_px = float2(tc_dy.y * to_centre.x - tc_dy.x * to_centre.y,
		tc_dx.x * to_centre.y - tc_dx.y * to_centre.x) / (abs(det) > 1e-12 ? det : 1e-12);
	const float2 bore_tc = (screen_tc + centre_px * output_res.zw - 0.5) / zoom + 0.5;
	const float2 tan_half = float2(ogse_c_screen.z / max(ogse_c_screen.y, 0.01), ogse_c_screen.z);
	return reticle_atlas_angle(atlas, reticle_atlas_frame_tangent(frame_tc, tan_half),
		reticle_atlas_frame_tangent(bore_tc, tan_half));
}

// Where the 3DSS shaders sample a sight's reticle bitmap: reticle_tc (scaled by the focal plane) and the lens
// coordinate. A bitmap reticle draws there; an atlas in 3dss scale mode follows them (reticle_atlas_view).
void scope_legacy_reticle_tcs(Scope S, out float2 reticle_tc, out float2 reticle_lens_tc)
{
	float RETICLE_SIZE = s3ds_param_1.x;
	int FFP = s3ds_param_1.w;
	float RETICLE_PROJECT = 10;

	if (RETICLE_TYPE == RT_FLAT_SCREEN)
		RETICLE_PROJECT = 0;

	float current_zoom = (curMag()-minMag()) * 0.4 + 1.0;

	float3x3 TBN = cotangent_frame(S.v_N + 0.0039, S.v_P + 0.0039, S.tc0.xy);
	float3 V_tangent = normalize(float3(dot(-S.v_P, TBN[0]), dot(-S.v_P, TBN[1]), dot(-S.v_P, TBN[2])));

	float2 t_field = svp_reticle_t_field(S, V_tangent.xy);
	reticle_lens_tc = project(S.tc0, t_field, RETICLE_PROJECT, RETICLE_SIZE);
	reticle_tc = project(S.tc0, t_field, RETICLE_PROJECT, RETICLE_SIZE * (FFP || RETICLE_TYPE == RT_GIPERON || RETICLE_TYPE == RT_DFP ? current_zoom : 1));

	if (RETICLE_TYPE == RT_SCREEN || RETICLE_TYPE == RT_FLAT_SCREEN) {
		reticle_lens_tc = S.tc0;
		reticle_tc = S.tc0;
	}

	if (RETICLE_TYPE != RT_FLAT_SCREEN)
	{
		// the pip tangent drives the fisheye so the stock edge term survives true pip
		float2 fish = fisheye(S.tc0, (svp_reticle_tangent(S, V_tangent.xy) * svp_effective_mas(mas_scale()))) / current_zoom;
		reticle_lens_tc += fish;
		reticle_tc += fish;
	}

	svp_reticle_flip(reticle_tc, reticle_lens_tc, S);
	const float reticle_scale = RETICLE_SIZE
		* (FFP || RETICLE_TYPE == RT_GIPERON || RETICLE_TYPE == RT_DFP ? current_zoom : 1);
	svp_reticle_follow_barrel(
		reticle_tc, reticle_lens_tc, reticle_scale, RETICLE_SIZE,
		RETICLE_PROJECT);
}

// The 1P59 dial's fraction of the zoom (reticle_dial_angle). True PiP minMag is a moving anchor: the fraction is
// rebuilt in magnification space, then mapped to the measured 1p59 glyph angles.
float scope_dial_fraction()
{
	float fraction = zoomPercent();
	if (shader_scope_params.w < -1.5)
	{
		const float current_zoom = (curMag() - minMag()) * 0.4 + 1.0;
		const float dial_max = maxMag() * current_zoom / curMag();
		if (dial_max > 1.0)
			fraction = dial_angle_fraction(3.0 + 7.0 * saturate((current_zoom - 1.0) / (dial_max - 1.0)));
	}
	return fraction;
}

// transmission: the lens's exit-pupil transmission, evaluated once by the caller (the compiler overflows its stack
// when a shader evaluates it twice)
float4 scope_bitmap_reticle(Scope S, float transmission) {
	float RETICLE_SIZE = s3ds_param_1.x;
	float EYE_RELIEF = s3ds_param_1.y;
	float EXIT_PUPIL = s3ds_param_1.z;
	int FFP = s3ds_param_1.w;

	float REFLECTION_HUE = s3ds_param_2.x;
	int MIN_ZOOM_1X = s3ds_param_2.z;

	float DIRT_INTENSITY = decoded_dirt_intensity();
	float CHROMA_POWER = s3ds_param_3.w;

	if (!SETTING(SETTINGS, ST_CHROMATISM) || svp_reticle_kill_chroma()) {
		CHROMA_POWER = 0;
	}

	float lum = current_lum();
	float acog_fiber = svp_reticle_acog_fiber(lum);

	float2 reticle_tc, reticle_lens_tc;
	scope_legacy_reticle_tcs(S, reticle_tc, reticle_lens_tc);

    float4 mark_texture = float4(0, 0, 0, 0);
	if (reticle_tc.x >= 0 && reticle_tc.x <= 1 && reticle_tc.y >= 0 && reticle_tc.y <= 1)
	{
		if (RETICLE_TYPE == RT_MARK_MAGNIFIER)
		{
			reticle_tc = mark_adjust(reticle_tc);
		}
		float2 tc = SCOPECOORD_TO_TEXCOORD(clamp(reticle_tc, 0, 1));
		mark_texture = s_reticle.Sample(smp_base, tc);

		// Try and determine whether the texture is transparent or not.
		//    Sample from two coordinates that are likely to be "transparent" in case mipmaps are missing
		bool opaque1 = s_reticle.SampleLevel(smp_base, float2(0.233, 0.1), 24).a > .99;
		bool opaque2 = s_reticle.SampleLevel(smp_base, float2(0.1, 0.233), 24).a > .99;
		if (opaque1 && opaque2)
			mark_texture.a = length(mark_texture.xyz);
	}
	
	if (RETICLE_TYPE == RT_DFP)
	{
		float4 tex_ffp = float4(0,0,0,0);
		float4 tex_sfp = float4(0,0,0,0);

		if (reticle_tc.x >= 0 && reticle_tc.x <= 1 && reticle_tc.y >= 0 && reticle_tc.y <= 1)
			tex_ffp = s_reticle.Sample(smp_base, SCOPECOORD_TO_TEXCOORD(clamp(reticle_tc, 0, 1)));;

		if (reticle_lens_tc.x >= 0 && reticle_lens_tc.x <= 1 && reticle_lens_tc.y >= 0 && reticle_lens_tc.y <= 1)
			tex_sfp = s_reticle.Sample(smp_base, SCOPECOORD_TO_TEXCOORD(clamp(reticle_lens_tc, 0, 1)));

		float vis = max(tex_ffp.r, tex_sfp.g);
		float led = tex_sfp.b;

		float4 base_lines = float4(0.0, 0.0, 0.0, vis);
		float4 led_layer  = float4(markswitch_color.rgb, led);

		mark_texture = rgba_blend(base_lines, led_layer);
	}

	if (RETICLE_TYPE == RT_DFP_ALT || RETICLE_TYPE == RT_DFP_ALT2)
	{
		float4 tex_ffp = float4(0,0,0,0);
		float4 tex_sfp = float4(0,0,0,0);

		if (reticle_tc.x >= 0 && reticle_tc.x <= 1 && reticle_tc.y >= 0 && reticle_tc.y <= 1)
			tex_ffp = s_reticle.Sample(smp_base, SCOPECOORD_TO_TEXCOORD(clamp(reticle_tc, 0, 1)));

		if (reticle_lens_tc.x >= 0 && reticle_lens_tc.x <= 1 && reticle_lens_tc.y >= 0 && reticle_lens_tc.y <= 1)
			tex_sfp = s_reticle.Sample(smp_base, SCOPECOORD_TO_TEXCOORD(clamp(reticle_lens_tc, 0, 1)));

		float vis = max(tex_ffp.r, tex_sfp.g);
		float led = tex_ffp.b;
		float illum_enabled = saturate(dot(markswitch_color.rgb, 1.0));

		float4 base_lines = float4(0.0, 0.0, 0.0, vis);
		float4 led_layer  = float4(markswitch_color.rgb, led * illum_enabled);

		mark_texture = rgba_blend(base_lines, led_layer);
	}

	float4 giperon_sfp = float4(0, 0, 0, 0);
	if (RETICLE_TYPE == RT_GIPERON)
	{
		mark_texture = mark_texture.r;
		float finder = s_reticle.Sample(smp_base, SCOPECOORD_TO_TEXCOORD(clamp(reticle_lens_tc, 0, 1))).g;
		float angle = reticle_dial_angle(scope_dial_fraction());
		float2 tc = reticle_lens_tc - 0.5;
		tc = float2(tc.x * cos(angle) - tc.y * sin(angle), tc.x * sin(angle) + tc.y * cos(angle));
		tc += 0.5;
		float numbers = s_reticle.Sample(smp_base, SCOPECOORD_TO_TEXCOORD(clamp(tc, 0, 1))).b;
		giperon_sfp = float4(0, 0, 0, max(finder, numbers));
	}

	if (RETICLE_TYPE == RT_ACOG)
	{
		float3 black = float3(0, 0, 0);
		float3 text = float3(0.3, 0.3, 0.3);
		float tritium_lum = 0.15;
		mark_texture = rgba_blend(rgba_blend(float4(black, mark_texture.r), float4(markswitch_color.rgb * max(tritium_lum, acog_fiber * 2), mark_texture.g)), float4(text, mark_texture.b * lum));
	}
	
	if (RETICLE_TYPE == RT_LED || RETICLE_TYPE == RT_GIPERON)
	{
		mark_texture = float4(markswitch_color.rgb, mark_texture.a);
		giperon_sfp = float4(markswitch_color.rgb, giperon_sfp.a);
	}
	
	if (RETICLE_TYPE == RT_SPECTER)
	{
		float3 black = float3(0, 0, 0);
		float4 light = float4(0, 0, 0, 0);
		if (markswitch_current.x == 1)
			light = float4(markswitch_color.rgb, mark_texture.g);
		if (markswitch_current.x == 2)
			light = float4(markswitch_color.rgb, mark_texture.b);
		
		mark_texture = rgba_blend(float4(black, mark_texture.r), light);
	}
	
	// Reticle-space edge masking belongs to the legacy 3DSS projection. True PiP
	// gets its tube boundary and eye-box loss from the dedicated physical passes.
	const bool true_pip = shader_scope_params.w < -1.5;
	float4 mark_shadow = true_pip
		? float4(0, 0, 0, 0)
		: sample_shadow(reticle_lens_tc, 0.05);
	if (RETICLE_TYPE == RT_SCREEN || RETICLE_TYPE == RT_FLAT_SCREEN){
		mark_shadow *= 0.0;
	} else {
		if ((distance(S.sfp,S.center) > S.radius) && (RETICLE_TYPE != RT_MARK_MAGNIFIER))
			mark_shadow = float4(0,0,0,1);
	}
	
	// LED-illuminated inside walls
	const bool inside_excluded = RETICLE_TYPE == RT_SCREEN
		|| RETICLE_TYPE == RT_FLAT_SCREEN
		|| RETICLE_TYPE == RT_SPECTER
		|| RETICLE_TYPE == RT_ACOG
		|| RETICLE_TYPE == RT_MARK_MAGNIFIER;
	const bool uses_authored_inside = svp_scope_uses_authored_inside(FFP, inside_excluded);
	const float2 inside_tc = SCOPECOORD_TO_TEXCOORD(
		clamp((reticle_tc - 0.5) * 0.62 + 0.5, 0, 1));
	float4 inside = 0;
	if (uses_authored_inside) {
		inside = svp_sample_authored_inside(inside_tc);
		inside.rgb = markswitch_color.rgb * inside.r;
	}
	
	if (RETICLE_TYPE == RT_LED_MASKED)
	{
		float3 black = float3(0, 0, 0);
		mark_texture = rgba_blend(float4(black, mark_texture.r), float4(markswitch_color.rgb, mark_texture.g));
	}
	
	if (!SETTING(SETTINGS, ST_SEE_THROUGH))
	{
		mark_texture.rgb *= zoomRotateFactor();
		giperon_sfp.rgb *= zoomRotateFactor();
	}
	
	#ifdef SCOPE_REFERENCE_INSIDE_COVERAGE
	SCOPE_REFERENCE_INSIDE_COVERAGE(float4(inside.rgb*1.5,saturate(inside.a*1.5)));
	#endif
	#ifndef SCOPE_REFERENCE_MARK_ONLY
	float4 final_scope = float4(0, 0, 0, 0);	
	final_scope = rgba_blend(final_scope, mark_shadow);
	final_scope = rgba_blend(final_scope, inside*1.5);	
	mark_texture = rgba_blend(mark_texture, final_scope);
	#endif

	float4 result = RETICLE_TYPE == RT_GIPERON
		? rgba_blend(mark_texture, giperon_sfp)
		: mark_texture;
	svp_reticle_washout(result, lum);
	// 3DSS 5.02 thermal battery: the reticle is part of the display, it dims with it and goes out when flat
	if (scope_battery_powered())
	{
		if (SCOPE_BATTERY_CHARGE <= THERMAL_CRITICAL_CHARGE)
			result.a = 0;
		else
			result.rgb = lerp(THERMAL_POWERED_OFF_COLOR, result.rgb, thermal_power_signal(SCOPE_BATTERY_CHARGE));
	}
	#ifndef SCOPE_REFERENCE_MARK_ONLY
	if (svp_physical_optics_active() && RETICLE_TYPE != RT_FLAT_SCREEN)
		result.a *= transmission;
	#endif
	return result;
}

float4 scope_custom_reticle(Scope S)
{
	// Atlas reticle angle, taken outside any branch so its screen derivatives stay defined. The Reference
	// glyph pass rasterises in the UV chart and only keeps the inside coverage, it never draws marks.
#ifndef SCOPE_REFERENCE_GLYPH_LAYER
	const ReticleAtlas atlas = reticle_atlas_open(s_reticle);
	float2 reticle_angle;
	float2 reticle_image_tc = S.sfp; // true PiP: the eyepiece capture coordinate of the image pass (SCOPECOORD_TO_TEXCOORD)
	if (shader_scope_params.w < -1.5 && svp_env.w > 0.5 && ddy(S.tc0.y) < 0.0)
		reticle_image_tc.y = 1.0 - reticle_image_tc.y;
	if (svp_reticle_projection.x > 0.0)
	{
		float2 reticle_uv = S.tc0;
		if (shader_scope_params.w < -1.5 && svp_env.w > 0.5 && ddy(S.tc0.y) < 0.0)
			reticle_uv.y = 1.0 - reticle_uv.y;
		const float2 reticle_bore = 0.5 + (svp_physical_optics_active() && svp_barrel_alignment.z > 0.5
			? svp_barrel_alignment.xy : 0.0);
		reticle_angle = reticle_atlas_angle(atlas,
			reticle_atlas_projected_tangent(reticle_uv, svp_reticle_projection),
			reticle_atlas_projected_tangent(reticle_bore, svp_reticle_projection));
	}
	else
		reticle_angle = scope_reticle_frame_angle(S, atlas, reticle_image_tc);
	float2 reticle_legacy_tc, reticle_legacy_lens_tc;
	scope_legacy_reticle_tcs(S, reticle_legacy_tc, reticle_legacy_lens_tc);
	const ReticleView reticle_view = reticle_atlas_view_dial(reticle_atlas_view(atlas, reticle_angle,
		curMag() / max(svp_optics.w, 1.0), reticle_legacy_tc, reticle_legacy_lens_tc), reticle_dial_angle(scope_dial_fraction()));
	// the lens's exit-pupil transmission, once: the reticle and the capture's light share it
	float reticle_transmission = 1.0;
	#ifndef SCOPE_REFERENCE_MARK_ONLY
	if (svp_physical_optics_active())
		reticle_transmission = svp_exit_pupil_transmission(svp_scope_lens_tc(S.w_P));
	#endif
#ifdef RETICLE_ATLAS_EMISSION_PASS
	// the glow only: the lens's atlas reticle, and under PiP a reflex the capture shows, both under the PiP lens's
	// dirt (the Reference image has dirt of its own)
	float reticle_dirt = 0.0;
	#ifndef SCOPE_REFERENCE_MARK_ONLY
	reticle_dirt = scope_reticle_dirt(S);
	#endif
	float4 result = 0;
	if (atlas.recognised)
		result = scope_atlas_reticle(0, reticle_transmission, reticle_dirt, atlas, reticle_view);
	#ifndef SCOPE_REFERENCE_MARK_ONLY
	scope_reticle_capture_light(S, reticle_transmission, reticle_dirt, result);
	#endif
	if (!any(result != 0) && !any(reticle_atlas_glow_source != 0))
		discard;
	return result;
#else
	float4 result;
	if (atlas.recognised)
	{
		const float3 reticle_background = scope_reticle_background(S, reticle_image_tc);
		result = scope_atlas_reticle(reticle_background, reticle_transmission, 0.0, atlas, reticle_view);
	}
	else
		result = scope_bitmap_reticle(S, reticle_transmission);
	return result;
#endif
#else
	return scope_bitmap_reticle(S, 1.0);
#endif
}
