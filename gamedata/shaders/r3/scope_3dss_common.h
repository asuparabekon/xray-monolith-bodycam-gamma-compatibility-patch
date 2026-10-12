// PIP_COMPAT_PATCH scope_3dss_common 20261007 3dss502 fusion/battery decode
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

#ifndef SCOPE_3DSS_COMMON_H
#define SCOPE_3DSS_COMMON_H


#include "scope_common.h"

#include "common.h"
#include "nv_utils.h"
#include "thermal_utils.h"

#define REFLECTIONS_FACTOR 0.3
#define SPECULAR_FACTOR 2.0

#define PI 3.1415926

#define RT_NONE 0
#define RT_LED 1
#define RT_LED_MASKED 2
#define RT_ACOG 3
#define RT_SPECTER 4
#define RT_GIPERON 5
#define RT_SCREEN 6
#define RT_ADDITIVE 7
#define RT_FLAT_SCREEN 8
#define RT_DFP 9
#define RT_MARK_MAGNIFIER 10

#define IT_NONE 0
#define IT_NV 1
#define IT_THERMAL 2
#define IT_THERMAL_COLOR 3

#define ST_NVG_BLUR 0
#define ST_PARALLAX_SHADOW 1
#define ST_SEE_THROUGH 2
#define ST_THERMAL_PIXELATION 3
#define ST_CHROMATISM 4
#define ST_FUSION_OUTLINE 5

#define SETTING(settings, shift) (settings & (1 << shift))

#define SETTINGS int(floor(s3ds_param_4.w))
#define IMAGE_TYPE int(s3ds_param_3.x)
#define LENS_COLOR s3ds_param_4.xyz
#define SCOPE_NVG_FRACTION frac(s3ds_param_4.w)
#define SCOPE_NVG_GENERATION clamp(int(floor(SCOPE_NVG_FRACTION * 10.0 + 0.001)), 1, 4)
#define SCOPE_NVG_GAIN max(floor((SCOPE_NVG_FRACTION - float(SCOPE_NVG_GENERATION) * 0.1) * 1000.0 + 0.5) / 10.0, 0.1)
#define SCOPE_FUSION_CODE (int(floor(SCOPE_NVG_FRACTION * 10000.0 + 0.5)) % 10)
// GAMMA Patch: z_3d_scopes packs (fusion colour - 1) + 4 * custom off texture, as the 5.02 lens mesh shader decodes it
#define SCOPE_FUSION_COLOR (1 + (SCOPE_FUSION_CODE % 4))
#define SCOPE_BATTERY_TEXTURE_OFF (SCOPE_FUSION_CODE >= 4)
#define SCOPE_BATTERY_CHARGE clamp((s3ds_param_2.y - 1.251) * 1000.0, -1.0, 1.0)

#define RETICLE_TYPE int(s3ds_param_3.y)

Texture2D s_inside;
Texture2D s_dirt;
Texture2D s_heat_map;
Texture2D s_powered_off;
TextureCube s_reflection;

float4 sampleScopeTex(float2 tc) {
	return s_image.Sample(smp_base, SCOPECOORD_TO_TEXCOORD(tc));
}

float4 sample_shadow(float2 tc, float shadow_width)
{
	float a = smoothstep(0.5 - shadow_width, 0.5, distance(tc, float2(0.5, 0.5))); 
	return float4(0, 0, 0, a);
}

float4 sample_zoom_switch_shadow(float2 tc)
{
	float shift = zoomPercent();
	float a = smoothstep(1 - 0.1, 1, distance(tc, float2(0.5 + shift * 3, 0.5)));
	float b = smoothstep(1 - 0.4, 1, distance(tc, float2(-2.5 + shift * 3, 0.5)));
	return float4(0, 0, 0, a * b);
}

float2 project(float2 tc, float2 tangent, float distance, float size)
{
    float2 parallax_tc = tc - tangent * distance;
	parallax_tc.x = (parallax_tc.x + (size - 1) / 2) / size;
	parallax_tc.y = (parallax_tc.y + (size - 1) / 2) / size;
    return parallax_tc;
}

float3 chroma_sample(float2 lens_tc, float2 back_tc, float power)
{
    float3 color_sum = float3(0, 0, 0);
    float3 weight_sum = float3(0, 0, 0);
    
    for(int i = 0; i <= 16; ++i)
    {
        float step = i / 16.0;

        float2 delta = lens_tc - 0.5;
		float zoom_multiplier = min(1, 0.009 * curMag());
        delta = sign(delta) * pow(abs(delta), 3.5) * (2 * power + zoom_multiplier);
        float2 coord = back_tc + (step - 0.5) * delta;
        
        float3 weight = float3(step, 1.0 - abs(step + step - 1.0), 1.0 - step);
        weight = lerp(float3(0.5, 0.5, 0.5), weight, 2.0);
        
        float3 color = sampleScopeTex(coord).rgb;
        color_sum += color * color * weight;
        
        weight_sum += weight;
    }

    return max(0.0001, sqrt(color_sum / weight_sum));
}

float4 blur_sample(float2 uv)
{
    float4 color = float4(0, 0, 0, 0);
	float blur_amount = 0.006;
	float dither_amount = 0.006;

	float2 offsets[9] = {
		float2(-1, -1), float2(0, -1), float2(1, -1),
		float2(-1,  0), float2(0,  0), float2(1,  0),
		float2(-1,  1), float2(0,  1), float2(1,  1)
	};
	float weights[9] = {
		1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
		2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
		1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
	};

	// Generate a small random offset for dithering
	float2 dither = float2(
		(frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453) - 0.5) * dither_amount,
		(frac(sin(dot(uv, float2(93.9898, 67.345))) * 43758.5453) - 0.5) * dither_amount
	);

	for (int i = 0; i < 9; i++)
	{
		color += s_image.Sample(smp_base, uv + offsets[i] * blur_amount + dither) * weights[i];
	}

	return color;
}

float3 back_image_sample(float2 lens_tc, float2 back_tc, float power, bool blur, bool chroma)
{
    if (blur) {
		return blur_sample(back_tc);
	}
	
	if (chroma) {
		return chroma_sample(lens_tc, back_tc, power);
	}
	
	return sampleScopeTex(back_tc).rgb;
}

float scope_nvg_heat(float2 tc)
{
	float2 heat_pos = saturate(tc) * (screen_res.xy - 1.0);
	float3 heat = s_heat.Load(int3(heat_pos, 0), 0).rgb;
	return max(heat.r, max(heat.g, heat.b));
}

float scope_nvg_contour(float2 tc, float radius)
{
	float2 pixel = radius / screen_res.xy;
	float2 tc_left = saturate(tc - float2(pixel.x, 0.0));
	float2 tc_right = saturate(tc + float2(pixel.x, 0.0));
	float2 tc_up = saturate(tc - float2(0.0, pixel.y));
	float2 tc_down = saturate(tc + float2(0.0, pixel.y));

	float heat_center = scope_nvg_heat(tc);
	float heat_left = scope_nvg_heat(tc_left);
	float heat_right = scope_nvg_heat(tc_right);
	float heat_up = scope_nvg_heat(tc_up);
	float heat_down = scope_nvg_heat(tc_down);
	float heat_edge = max(max(abs(heat_center - heat_left), abs(heat_center - heat_right)),
		max(abs(heat_center - heat_up), abs(heat_center - heat_down)));
	float heat_nearby = max(heat_center, max(max(heat_left, heat_right), max(heat_up, heat_down)));

	gbuffer_data gbd_center = gbuffer_load_data(tc, tc * screen_res.xy, 0);
	gbuffer_data gbd_left = gbuffer_load_data(tc_left, tc_left * screen_res.xy, 0);
	gbuffer_data gbd_right = gbuffer_load_data(tc_right, tc_right * screen_res.xy, 0);
	gbuffer_data gbd_up = gbuffer_load_data(tc_up, tc_up * screen_res.xy, 0);
	gbuffer_data gbd_down = gbuffer_load_data(tc_down, tc_down * screen_res.xy, 0);
	float3 center_normal = normalize(gbd_center.N + 0.0001);
	float normal_left = 1.0 - saturate(dot(center_normal, normalize(gbd_left.N + 0.0001)));
	float normal_right = 1.0 - saturate(dot(center_normal, normalize(gbd_right.N + 0.0001)));
	float normal_up = 1.0 - saturate(dot(center_normal, normalize(gbd_up.N + 0.0001)));
	float normal_down = 1.0 - saturate(dot(center_normal, normalize(gbd_down.N + 0.0001)));
	float normal_edge = max(max(normal_left, normal_right), max(normal_up, normal_down));
	float depth_scale = max(abs(gbd_center.P.z), 1.0);
	float depth_left = abs(gbd_center.P.z - gbd_left.P.z);
	float depth_right = abs(gbd_center.P.z - gbd_right.P.z);
	float depth_up = abs(gbd_center.P.z - gbd_up.P.z);
	float depth_down = abs(gbd_center.P.z - gbd_down.P.z);
	float depth_edge = max(max(depth_left, depth_right), max(depth_up, depth_down)) / depth_scale;
	float contour = max(heat_edge * 5.0, normal_edge * 1.35 + saturate(depth_edge * 5.0));
	return saturate(contour) * smoothstep(0.01, 0.10, heat_nearby);
}

// GAMMA Patch: the 5.02 thermal battery, white/green fusion and packed dirt decode of
// models_scope_reticle.ps, shared here so the PiP lens composite reads the same lanes.
#define THERMAL_LOW_POWER_THRESHOLD 0.15
#define THERMAL_CRITICAL_CHARGE 0.0001
#define THERMAL_FLICKER_RATE 7.0
#define THERMAL_FLICKER_MIN_BRIGHTNESS 0.75
#define THERMAL_POWERED_OFF_COLOR float3(0.003, 0.004, 0.003)

float thermal_power_signal(float charge)
{
	if (charge <= THERMAL_CRITICAL_CHARGE)
		return 0.0;
	if (charge >= THERMAL_LOW_POWER_THRESHOLD)
		return 1.0;

	float pulse = floor(timers.x * THERMAL_FLICKER_RATE);
	float noise = frac(sin(pulse * 12.9898 + 4.1414) * 43758.5453);
	return lerp(THERMAL_FLICKER_MIN_BRIGHTNESS, 1.0, noise);
}

// A live thermal display that runs on a battery (-1 = the sight has no battery)
bool scope_battery_powered()
{
	return (IMAGE_TYPE == IT_THERMAL || IMAGE_TYPE == IT_THERMAL_COLOR)
		&& markswitch_current.x < 2 && SCOPE_BATTERY_CHARGE > -0.5;
}

int packed_white_fusion_value()
{
	return int(floor(s3ds_param_3.z * 102.0 + 0.5));
}

float white_fusion_fade()
{
	return float(packed_white_fusion_value() % 101) / 100.0;
}

float decoded_dirt_intensity()
{
	if (SCOPE_FUSION_COLOR != 3)
		return s3ds_param_3.z;
	return floor(float(packed_white_fusion_value()) / 101.0) / 100.0;
}

float3 apply_nvg(float2 tc, float3 img)
{
	// Adapted from Beef's Shader Based NVGs. The scope mesh supplies the mask,
	// so only generation, gain and image-tube processing are applied here.
	int generation = SCOPE_NVG_GENERATION;
	float gain = SCOPE_NVG_GAIN;

	float3 tube_color = generation == 1
		? float3(0.7, 1.0, 0.6)
		: generation == 2
			? float3(0.5, 1.0, 0.4)
			: float3(0.2, 1.0, 0.9);
	float crt_strength = generation == 1 ? 0.05 : generation == 2 ? 0.07 : 0.10;
	float noise_strength = generation == 1 ? 0.15 : generation == 2 ? 0.10 : 0.08;
	float scintillation = generation == 1 ? 0.9990 : generation == 2 ? 0.9991 : 0.9992;
	float sharpen_strength = generation == 1 ? 0.0 : generation == 2 ? 0.25 : 0.50;
	if (generation == 4)
	{
		tube_color = float3(1.0, 0.70, 0.40);
		crt_strength = 0.0;
		noise_strength = 0.0;
		scintillation = 1.0;
		sharpen_strength = 0.50;
	}

	float luma = dot(max(img, 0.0), float3(0.299, 0.587, 0.114));
	float2 pixel = 1.0 / screen_res.xy;
	float neighbor_luma = 0.25 * (
		dot(s_image.SampleLevel(smp_base, tc + float2(pixel.x, 0), 0).rgb, float3(0.299, 0.587, 0.114))
		+ dot(s_image.SampleLevel(smp_base, tc - float2(pixel.x, 0), 0).rgb, float3(0.299, 0.587, 0.114))
		+ dot(s_image.SampleLevel(smp_base, tc + float2(0, pixel.y), 0).rgb, float3(0.299, 0.587, 0.114))
		+ dot(s_image.SampleLevel(smp_base, tc - float2(0, pixel.y), 0).rgb, float3(0.299, 0.587, 0.114)));
	luma += (luma - neighbor_luma) * sharpen_strength;
	// Beef's combine pass amplifies scene luminance before applying tube gain.
	luma *= 4.0 * gain;

	float2 grain_tc = tc - fmod(tc, float2(3.0, 3.0) / screen_res.xy);
	float grain = frac(sin(dot(grain_tc, float2(12.0, 78.0) + timers.x)) * 22738.0);
	float scintillation_jitter = frac(sin(dot(tc, float2(39.346, 11.135) + timers.x)) * 78372.0);
	luma += (grain - 0.5) * 2.0 * noise_strength * sqrt(gain);
	if (scintillation_jitter > scintillation - ((1.0 - scintillation) * (gain - 1.0)))
	{
		luma = 1.0;
	}

	float3 nvg = saturate(luma) * tube_color;
	float scanline = clamp(0.35 + 0.35 * sin(tc.y * screen_res.y * 2.0), 0.0, 1.0);
	float3 crt = nvg * (0.4 + 0.7 * pow(scanline, 1.7));
	nvg = lerp(nvg, crt, crt_strength);
	if (nvg.g >= 0.95)
	{
		nvg = pow(nvg.ggg, 2.0);
	}

	if ((SCOPE_FUSION_COLOR == 1 || SCOPE_FUSION_COLOR == 2)
		&& SETTING(SETTINGS, ST_FUSION_OUTLINE)
		&& (SCOPE_BATTERY_CHARGE < -0.5 || SCOPE_BATTERY_CHARGE > THERMAL_CRITICAL_CHARGE))
	{
		float core = smoothstep(0.12, 0.42, scope_nvg_contour(tc, 0.70));
		float halo = smoothstep(0.035, 0.24, scope_nvg_contour(tc, 2.25));
		float3 outline_color = SCOPE_FUSION_COLOR == 2
			? float3(1.0, 0.16, 0.08)
			: float3(1.0, 0.78, 0.48);
		float outline_alpha = saturate(core * 0.54 + halo * 0.11);
		nvg = lerp(nvg, outline_color, outline_alpha);
		nvg += outline_color * halo * 0.12;
	}

	return saturate(nvg);
}

float3 apply_white_fusion(float2 tc, float3 image)
{
	int fusion_color = SCOPE_FUSION_COLOR;
	if ((fusion_color != 3 && fusion_color != 4)
		|| !SETTING(SETTINGS, ST_FUSION_OUTLINE)
		|| (SCOPE_BATTERY_CHARGE >= -0.5 && SCOPE_BATTERY_CHARGE <= THERMAL_CRITICAL_CHARGE))
		return image;

	float core = smoothstep(0.12, 0.42, scope_nvg_contour(tc, 0.70));
	if (fusion_color == 4)
	{
		float halo = smoothstep(0.035, 0.24, scope_nvg_contour(tc, 2.25));
		float heat_fill = smoothstep(0.05, 0.32, scope_nvg_heat(tc));
		float3 green = float3(0.20, 0.92, 0.30);
		image = lerp(image, green, heat_fill * 0.144);
		image = lerp(image, green, saturate(core * 0.18 + halo * 0.027));
		return saturate(image + green * halo * 0.0135);
	}

	float outline_alpha = saturate(core * white_fusion_fade());
	return lerp(image, float3(1.0, 1.0, 1.0), outline_alpha);
}

float3 lcd_effect(int2 hpos)
{
	float pb = 0.4;
    float3 lcdColor = float3(pb, pb, pb);
	
    int px = fmod(hpos.x, 3);
	if (px == 1)
		lcdColor.r = 1;
    else if (px == 2)
		lcdColor.g = 1;
    else
		lcdColor.b = 1;
    
    float sclV = 0.25;
    if (int(fmod(hpos.y, 3)) == 0)
		lcdColor.rgb = float3(sclV, sclV, sclV);
	
	return lcdColor;
}

float2 fisheye_shift(float2 uv, float progress, float2 center)
{
	uv -= center;
    
	float ratio = 1;
	float pUvX = pow(uv.x * ratio, 2);
	float pUvY = pow(uv.y, 2);
	float pSum = pUvX + pUvY;
	float multiplier = 10 * (1 - progress);
	float strength = 1 - multiplier * pSum;
	uv *= strength;
    
	uv += center;
	return uv;
}

float2 fisheye(float2 tc, float2 tangent)
{
	float FISHEYE_STRENGTH = -0.4;
	float FISHEYE_PROJECT = 1;

	float fish_power = 1 + FISHEYE_STRENGTH * length(tangent);
	float2 fished = fisheye_shift(tc, fish_power, project(float2(0.5, 0.5), tangent, FISHEYE_PROJECT, 1.0));
	return fished - tc;
}

float current_lum()
{
	float lum_min = 0.85;
	float lum_max = 2; //3;
	float lum = s_tonemap.Load(int3(0, 0, 0)).x;  
	return clamp(1 - (lum - lum_min) / (lum_max - lum_min), 0, 1);
}

float4 rgba_blend(float4 b, float4 a)
{
	float na = a.a + b.a * (1 - a.a);
	float3 nc = na > 0 ? (a.rgb * a.a + b.rgb * b.a * (1 - a.a)) / na : float3(0, 0, 0);
	return float4(nc, na);
}

float sample_vignette(float2 tc)
{
    float dist = length(tc - 0.5);
    float vig = 1.0 - dist * 2;
    return pow(vig, dist * 1.0);
}

float4 sample_specular(float3 pnt, float3 normal, float3 light_direction)
{
	float w = pow(abs(dot(normalize(pnt + light_direction), normal)), 3072);
	return float4(L_sun_color.rgb * w, w);
}

float3 sample_lens_normalmap(float2 tc, float radius)
{
	float2 xy = (tc - 0.5) * 2;
	return float3(xy, sign(radius) * sqrt(pow(radius, 2) - dot(xy, xy)));
}

float3 sample_reflections(float2 tc, float parallax_factor, float radius, float dirt_factor, float3x3 TBNw_inv, float3 w_pos, float3 w_nrm, float lum, bool is_flat)
{
	float3 normalmap = sample_lens_normalmap(tc, radius);
	if (is_flat)
		normalmap = float3(0, 0, 1);
    float3 lensnormal = normalize(float3(dot(normalmap, TBNw_inv[0]), dot(normalmap, TBNw_inv[1]), dot(normalmap, TBNw_inv[2])));

	float3 reflections = s_reflection.SampleLevel(smp_base, reflect(normalize(normalize(w_pos - eye_position) - normalize(w_nrm) * parallax_factor), lensnormal), 0).rgb;
	if (!is_flat)
		reflections *= sample_vignette(tc);
	
	return reflections * normalize(0.3 + L_ambient.rgb + L_hemi_color.rgb);
}

float sample_reflections_weight(float dirt_factor, float3 w_pos, float3 w_nrm, float lum, bool see_through)
{
	float angle_factor = smoothstep(0, 0.03, (dot(normalize(w_pos - eye_position), normalize(w_nrm)) + 1) / 2);
	if (!see_through) {
		angle_factor = lerp(1, angle_factor, m_hud_params.x);
	}
	return
		angle_factor
		* smoothstep(0, 1, lum)
		* pow(1 - dirt_factor, 10)
		* REFLECTIONS_FACTOR;
}

float3 sample_specular(float2 tc, float parallax_factor, float radius, float specular_factor, float dirt_factor, float3x3 TBNw_inv, float3 w_pos, float3 w_nrm, bool see_through)
{
	float3 normalmap = sample_lens_normalmap(tc, radius);
	
	float angle_factor = smoothstep(0, 0.03, (dot(normalize(w_pos - eye_position), normalize(w_nrm)) + 1) / 2);
	//if (!see_through) {
	//	angle_factor = lerp(1, angle_factor, m_hud_params.x);
	//}

	float4 specular = sample_specular(
		normalize(normalize(w_pos - eye_position) - normalize(w_nrm) * parallax_factor),
		normalize(float3(dot(normalmap, TBNw_inv[0]), dot(normalmap, TBNw_inv[1]), dot(normalmap, TBNw_inv[2]))),
		normalize(L_sun_dir_w)
	) * 4;
	specular.w *= L_material.g * angle_factor;
	specular.w = min(1, specular.w);
	specular.w *= pow(1 - dirt_factor, 10);
	specular.w *= specular_factor;
	
	return specular.rgb * specular.a * normalize(0.3 + L_ambient.rgb + L_hemi_color.rgb);
}

float3 HSVtoRGB(float3 hsv)
{
	if (hsv.y == 0.0)
		return hsv.z;

	float i = floor(hsv.x * 6.0);
	float f = hsv.x * 6.0 - i;
	float p = hsv.z * (1.0 - hsv.y);
	float q = hsv.z * (1.0 - f * hsv.y);
	float t = hsv.z * (1.0 - (1.0 - f) * hsv.y);

	i = fmod(i, 6.0);

	if (i == 0.0)
		return float3(hsv.z, t, p);
	else if (i == 1.0)
		return float3(q, hsv.z, p);
	else if (i == 2.0)
		return float3(p, hsv.z, t);
	else if (i == 3.0)
		return float3(p, q, hsv.z);
	else if (i == 4.0)
		return float3(t, p, hsv.z);
	return float3(hsv.z, p, q);
}

float3 tonemap(float3 color, float3 v_pos)
{
    float fog = saturate(length(v_pos) * fog_params.w + fog_params.x);
    color = lerp(color, fog_color, fog);
	color = lerp(color, fog_color, saturate(fog * fog));

	color *= s_tonemap.Load(int3(0, 0, 0)).x;
	const float fWhiteIntensity = 1.7;
	const float fWhiteIntensitySQR = fWhiteIntensity * fWhiteIntensity;
    return (color * (1 + color / fWhiteIntensitySQR)) / (color + 1);
}

#endif
