// Reticle atlases: the self-describing texture a 3DSS reticle is drawn from, at the path of the bitmap it
// replaces. Atlas format 11 (vector reticles, layout in reticle_vector.h): settings, Mark Switch marks,
// illumination and emission, shared by the stock 3DSS lens, PiP, Reference and reflex sights
// (reticle_atlas_reflex.h). The only writer is the Bodycam engine's src/Layers/xrRender/reticle_vector (in game
// from SVG, or its command-line tool). Older atlases (the MSDF formats 2-8 and vector formats 9-10) are recognised
// and not drawn (magenta with r__reticle_msdf_debug). Evaluation only; every path composes the result its own way.
#ifndef RETICLE_ATLAS_H
#define RETICLE_ATLAS_H

#define RATLAS_ROLE_INK 1
#define RATLAS_ROLE_ILLUM 2
#define RATLAS_ROLE_PAINT 3
#define RATLAS_ROLE_GLOW 4 // paint that also lights up with the illumination
#define RATLAS_ROLE_LIGHT 5 // light only, unseen while the illumination is off (a fibre dot)
#define RATLAS_FLAG_ILLUMINATES 1
#define RATLAS_FLAG_PAINT 2
#define RATLAS_FLAG_HOLO 4
#define RATLAS_FLAG_FFP_PARTS 8  // parts that grow with the zoom (first focal plane)
#define RATLAS_FLAG_SFP_PARTS 16 // parts that keep their size (second focal plane)
#define RATLAS_FLAG_DIAL_PARTS 32 // parts that keep their size and turn with the zoom (a magnification dial)
#define RATLAS_DEFAULT_LUMINANCE 8.0
#define RATLAS_UNITS_PER_RADIAN_MOA 3437.74677
#define RATLAS_UNITS_PER_RADIAN_MRAD 1000.0

#ifndef RETICLE_ATLAS_PARAMS_DECLARED
#define RETICLE_ATLAS_PARAMS_DECLARED
// engine settings: x holographic level, y brightness, z holographic grain depth, w line weight (0 realistic .. 1 bold)
uniform float4 reticle_atlas_params;
// x 1 = the engine takes the reticle glow into its own target, y 1 = unusable atlases show magenta, w 1 = params are set
uniform float4 reticle_atlas_control;
// x 1 = the reticle glow targets are bound at slots 2 and 3 for this reflex draw, -1 = a reflex draw the engine
// repeats later: no reticle light now; y 1 = the lenses' copies of the glow targets hold the frame's reflex glow
uniform float4 reticle_atlas_reflex_glow;
#endif

struct ReticleAtlas
{
	bool recognised; // an atlas header is present: never treat the texture as a legacy bitmap
	bool valid;      // a format 11 atlas that passed every check (an older atlas never is)
	float2 dims;     // texels
	uint piece_count;
	uint curve_count;
	uint list_entries; // u16 entries in the grid's piece lists
	uint flags;
	uint unit;
	uint mark_count;    // Mark Switch marks (patterns) the atlas draws, at least 1
	bool drawing_colours; // the illumination takes the drawing's colours, not the sight's Mark Switch colour
	float calibrated_magnification;
	float legacy_span;   // units across one UV of the bitmap the atlas replaces (0 = true angular mapping)
	float luminance;     // lit core at the top brightness step, scene luminance
	float fibre;         // daylight lit: the lit core is this times the sky's luminance (0 = battery lit)
	float tritium;       // a constant lit core, scene luminance (0 = none)
	float glow_strength; // the glow's energy relative to the core
	uint step_count;     // authored brightness steps (0 = the Mark Switch colour's magnitude)
	uint2 steps;         // brightness * 255 of each Mark Switch mark from the first, four bytes per component
	float coord_scale;   // reticle units per coordinate step
	uint2 mask_size;     // holographic mask texels, 0 = no mask
	uint mask_row;
	float mask_mean;
	// a hologram's look (reticle_atlas_reflex.h): edge softness, level against an LED reticle, ghost level and offset
	// (units), colour fringe (units), low-step flicker depth
	float holo_softness, holo_level, holo_ghost_level, holo_fringe, holo_flicker;
	float2 holo_ghost_offset;
	// its grain mask (reticle_vector_grain): depth, units per repeat, shift per mm of eye movement, changes per second
	float grain_depth, grain_size, grain_shift, grain_rate;
};

// Where a pixel is on the reticle, per focal plane: the reticle angle (units, x right, y down) of the parts that
// grow with the zoom (ffp) and of those that keep their size (sfp), and their change per screen pixel. The dial
// parts sit at the sfp place turned by dial_angle (radians).
struct ReticleView
{
	float2 ffp, ffp_dx, ffp_dy;
	float2 sfp, sfp_dx, sfp_dy;
	float dial_angle;
};

struct ReticleMarks
{
	float ink;        // etched coverage, opaque black (includes unlit illuminated elements)
	float illum;      // coverage of the illuminated elements
	float3 illum_rgb; // their light and the light-only shapes', colour times coverage (white with the sight's colours)
	float paint;      // painted coverage
	float3 paint_rgb; // painted albedo, coverage weighted
	float3 glow_rgb;  // light of the paint that lights up, as illum_rgb
};

uint4 ratlas_texel(Texture2D t, uint x, uint y)
{
	return uint4(round(t.Load(int3(x, y, 0)) * 255.0));
}

#define RATLAS_STREAM_WIDTH 128 // descriptor stream texels per row

// descriptor stream texel k: (k % 128, 1 + k / 128)
uint4 ratlas_stream(Texture2D t, uint k)
{
	return ratlas_texel(t, k % RATLAS_STREAM_WIDTH, 1 + k / RATLAS_STREAM_WIDTH);
}

#define RATLAS_RECORD_TEXELS 9 // the settings record, 36 bytes, at the start of the descriptor stream

// byte i of the settings record, from its texels as reticle_vector_open loaded them
uint ratlas_record_byte(uint4 rec[RATLAS_RECORD_TEXELS], uint i)
{
	const uint4 v = rec[i >> 2];
	const uint c = i & 3;
	return c == 0 ? v.x : (c == 1 ? v.y : (c == 2 ? v.z : v.w));
}

uint ratlas_record_u16(uint4 rec[RATLAS_RECORD_TEXELS], uint i)
{
	return ratlas_record_byte(rec, i) | (ratlas_record_byte(rec, i + 1) << 8);
}

float ratlas_record_s8(uint4 rec[RATLAS_RECORD_TEXELS], uint i)
{
	const uint b = ratlas_record_byte(rec, i);
	return b >= 128 ? float(b) - 256.0 : float(b);
}

// Holographic sights widen the filter footprint by this much (reticle_atlas_reflex.h sets it)
static float ratlas_soften = 1.0;

float reticle_atlas_line_weight()
{
	return reticle_atlas_control.w > 0.5 ? saturate(reticle_atlas_params.w) : 0.0;
}

// The settings record (layout in reticle_vector.h): calibration, marks, colours, illumination, brightness steps,
// hologram and grain
void ratlas_read_record(uint4 rec[RATLAS_RECORD_TEXELS], inout ReticleAtlas a)
{
	a.calibrated_magnification = max(ratlas_record_u16(rec, 0) / 100.0, 0.01);
	a.coord_scale = ratlas_record_u16(rec, 2) / 16.0 / 32767.0;
	a.mark_count = ratlas_record_byte(rec, 4);
	a.drawing_colours = ratlas_record_byte(rec, 5) == 1;
	a.legacy_span = ratlas_record_u16(rec, 6) / 8.0;
	const uint luminance = ratlas_record_u16(rec, 8);
	a.luminance = luminance > 0 ? luminance / 16.0 : RATLAS_DEFAULT_LUMINANCE;
	a.glow_strength = ratlas_record_byte(rec, 10) / 32.0;
	a.step_count = min(ratlas_record_byte(rec, 12), 8u);
	a.steps = uint2(rec[3].y | rec[3].z << 8 | rec[3].w << 16 | rec[4].x << 24, rec[4].y | rec[4].z << 8 | rec[4].w << 16 | rec[5].x << 24);
	a.holo_softness = max(ratlas_record_byte(rec, 21) / 32.0, 1.0);
	a.holo_level = ratlas_record_byte(rec, 22) / 100.0;
	a.holo_ghost_level = ratlas_record_byte(rec, 23) / 255.0;
	a.holo_ghost_offset = float2(ratlas_record_s8(rec, 24), ratlas_record_s8(rec, 25)) / 8.0;
	a.holo_fringe = ratlas_record_byte(rec, 26) / 100.0;
	a.holo_flicker = ratlas_record_byte(rec, 27) / 255.0;
	a.grain_depth = ratlas_record_byte(rec, 28) / 255.0;
	a.grain_size = max(ratlas_record_u16(rec, 29) / 16.0, 0.0625);
	a.grain_shift = ratlas_record_byte(rec, 31) / 100.0;
	a.grain_rate = ratlas_record_byte(rec, 32) / 16.0;
	a.fibre = ratlas_record_byte(rec, 33) / 32.0;
	a.tritium = ratlas_record_u16(rec, 34) / 256.0;
}

bool ratlas_daylight_lit(ReticleAtlas a)
{
	return a.fibre > 0.0 || a.tritium > 0.0;
}

// The 3DSS Mark Switch colours are made for blending a bitmap; emitted as light they read pale, so the lit hue
// is saturated by raising each channel, relative to the brightest, to this power
#define RATLAS_HUE_SATURATION 3.0

float3 ratlas_hue(float3 colour)
{
	const float peak = max(colour.r, max(colour.g, colour.b));
	return peak > 0.0 ? pow(colour / peak, RATLAS_HUE_SATURATION) : float3(0, 0, 0);
}

#include "reticle_vector.h"

// A retired MSDF BC5 atlas: its header texels were 4x4 blocks carrying "RMSD" "FATL" in their endpoints
bool ratlas_retired_bc5(Texture2D t, uint w, uint h)
{
	if (w < 32 || h < 8)
		return false;
	const uint4 b0 = uint4(round(float4(t.Load(int3(0, 0, 0)).rg, t.Load(int3(0, 1, 0)).rg) * 255.0));
	const uint4 b1 = uint4(round(float4(t.Load(int3(4, 0, 0)).rg, t.Load(int3(4, 1, 0)).rg) * 255.0));
	return all(b0 == uint4(0x52, 0x4D, 0x53, 0x44)) && all(b1 == uint4(0x46, 0x41, 0x54, 0x4C));
}

ReticleAtlas reticle_atlas_open(Texture2D t)
{
	ReticleAtlas a = (ReticleAtlas)0;
	uint w, h, levels;
	t.GetDimensions(0, w, h, levels);
	if (w < 8 || h < 8)
		return a;
	const uint4 t0 = ratlas_texel(t, 0, 0), t1 = ratlas_texel(t, 1, 0);
	if (all(t0 == uint4(0x52, 0x45, 0x54, 0x49)) && all(t1.xyz == uint3(0x43, 0x4C, 0x45))) // "RETICLE"
	{
		a.recognised = true;
		if (t1.w == RVEC_VERSION)
			a = reticle_vector_open(t, a, w, h);
		return a;
	}
	// "RMSDFATL": the retired MSDF atlases and vector format 9
	a.recognised = (all(t0 == uint4(0x52, 0x4D, 0x53, 0x44)) && all(t1 == uint4(0x46, 0x41, 0x54, 0x4C))) || ratlas_retired_bc5(t, w, h);
	return a;
}

float ratlas_units_per_radian(ReticleAtlas a)
{
	return a.unit == 2 ? RATLAS_UNITS_PER_RADIAN_MRAD : RATLAS_UNITS_PER_RADIAN_MOA;
}

// Reticle angle (units, x right, y down) of a world direction given in tangent coordinates (x right, y up)
// relative to the bore direction, measured on the bore's tangent plane.
float2 reticle_atlas_angle(ReticleAtlas a, float2 view_tangent, float2 bore_tangent)
{
	const float3 f = normalize(float3(bore_tangent, 1.0));
	const float3 r = normalize(cross(float3(0.0, 1.0, 0.0), f));
	const float3 u = cross(f, r);
	const float3 d = float3(view_tangent, 1.0);
	const float3 p = d / dot(d, f);
	return float2(dot(p, r), -dot(p, u)) * ratlas_units_per_radian(a);
}

// View tangent (x right, y up) of a frame UV, tan_half = tangent of the half FOV along x and y.
float2 reticle_atlas_frame_tangent(float2 frame_uv, float2 tan_half)
{
	return float2(2.0 * frame_uv.x - 1.0, 1.0 - 2.0 * frame_uv.y) * tan_half;
}

// View tangent of an image UV rendered with projection (1/_11, 1/_22, _31, _32): tangents per NDC unit and the
// principal point offset in NDC.
float2 reticle_atlas_projected_tangent(float2 uv, float4 projection)
{
	return (float2(2.0 * uv.x - 1.0, 1.0 - 2.0 * uv.y) - projection.zw) * projection.xy;
}

// The pixel's place on the reticle for both focal planes. true_angle: the world angle from the bore (units);
// magnification: the sight's optical magnification. With legacy_span (3dss scale mode) the reticle follows the
// bitmap it replaces instead: legacy_uv is where the 3DSS shaders sample that bitmap (scaled with the zoom as the
// sight's 3DSS focal plane says), legacy_lens_uv the lens coordinate the 3DSS dual focal plane bitmaps sample
// their fixed-size part at. Call it outside branches: it takes screen derivatives.
ReticleView reticle_atlas_view(ReticleAtlas a, float2 true_angle, float magnification, float2 legacy_uv, float2 legacy_lens_uv)
{
	ReticleView v;
	if (a.legacy_span > 0.0)
	{
		v.ffp = (legacy_uv - 0.5) * a.legacy_span;
		v.sfp = (legacy_lens_uv - 0.5) * a.legacy_span;
	}
	else
	{
		v.ffp = true_angle;
		v.sfp = true_angle * max(magnification, 1.0) / a.calibrated_magnification;
	}
	v.ffp_dx = ddx(v.ffp);
	v.ffp_dy = ddy(v.ffp);
	v.sfp_dx = ddx(v.sfp);
	v.sfp_dy = ddy(v.sfp);
	v.dial_angle = 0.0;
	return v;
}

// The angle of a magnification dial, as 3DSS turns the dial of its 1P59 bitmap: `fraction` 0 at the sight's lowest
// magnification, 1 at its highest (each 3DSS path computes it its own way)
float reticle_dial_angle(float fraction)
{
	const float shift = 0.053; // the dial's offset at the lowest magnification
	return -3.14159265 * (fraction + shift) / (1.0 + shift);
}

// The view with its dial turned to `angle`
ReticleView reticle_atlas_view_dial(ReticleView v, float angle)
{
	v.dial_angle = angle;
	return v;
}

// The view moved by `offset` reticle units in both planes (a hologram's ghost and colour fringes)
ReticleView reticle_atlas_view_shift(ReticleView v, float2 offset)
{
	v.ffp += offset;
	v.sfp += offset;
	return v;
}

// All marks of Mark Switch mark `current` (markswitch_current, 0 based) at the view's place
ReticleMarks reticle_atlas_marks(Texture2D t, ReticleAtlas a, ReticleView v, float current)
{
	return reticle_vector_marks(t, a, v, current);
}

// The brightness of Mark Switch mark `current`: the atlas's authored step for this mark, else `fallback`
float ratlas_level(ReticleAtlas a, float current, float fallback)
{
	if (a.step_count == 0)
		return fallback;
	const uint i = min(uint(max(current, 0.0)), a.step_count - 1);
	return ((i < 4 ? a.steps.x : a.steps.y) >> (8 * (i & 3)) & 255) / 255.0;
}

// The illumination of Mark Switch mark `current`. x,y,z = hue at full value, w = brightness 0..1 (0 = off).
// With the sight's colours: the hue of the Mark Switch colour (markswitch_color.rgb, black = off; its alpha
// carries the dot offset), at the authored step or else the colour's magnitude. With the drawing's colours each
// lit shape brings its own hue (ReticleMarks.illum_rgb), so the hue here is white. A fibre and tritium sight is
// always lit, in the Mark Switch colour's hue if it has one.
float4 reticle_atlas_lighting(ReticleAtlas a, float4 colour, float current)
{
	if (ratlas_daylight_lit(a))
	{
		const bool hue = !a.drawing_colours && max(colour.r, max(colour.g, colour.b)) > 0.0;
		return float4(hue ? ratlas_hue(colour.rgb) : float3(1, 1, 1), 1.0);
	}
	if (a.drawing_colours)
		return float4(1, 1, 1, ratlas_level(a, current, 1.0));
	const float peak = max(colour.r, max(colour.g, colour.b));
	if (peak <= 0.0)
		return 0.0;
	return float4(ratlas_hue(colour.rgb), ratlas_level(a, current, saturate(peak)));
}

// The luminance of the sky light falling on an upward face, from the weather's ambient, sky and sun colours: what
// a fibre gathers
float ratlas_sky_luminance()
{
	const float3 light = max(L_ambient.rgb, 0.0) + max(L_hemi_color.rgb, 0.0) + max(L_sun_color.rgb, 0.0);
	return dot(light, float3(0.299, 0.587, 0.114));
}

// The lit core in scene luminance, unbounded and premultiplied by coverage. The engine scales it by the eye's
// exposure, spreads its glow and shoulders it toward white where it exceeds the display (the engine's 3DSS reticle glow).
float3 reticle_atlas_emission(ReticleAtlas a, ReticleMarks m, float4 illumination)
{
	if (illumination.w <= 0.0)
		return 0.0;
	const float3 core = max(m.illum_rgb, m.glow_rgb);
	const float brightness = reticle_atlas_control.w > 0.5 ? max(reticle_atlas_params.y, 0.0) : 1.0;
	const float level = ratlas_daylight_lit(a) ? a.fibre * ratlas_sky_luminance() + a.tritium
		: illumination.w * a.luminance;
	return illumination.rgb * (core * level * brightness);
}

// Light added straight into the scene when the engine has no glow target: no glow, the hue kept by a soft clip
float3 reticle_atlas_inline(float3 emission)
{
	const float peak = max(emission.r, max(emission.g, emission.b));
	return peak > 0.0 ? emission * ((1.0 - exp(-peak)) / peak) : 0.0;
}

// Outputs for the engine's glow targets: the core, and the same light weighted by the atlas's glow strength,
// which the engine spreads into the glow. Alpha is 1 where there is light and 0 elsewhere, so a source-alpha
// blend (the stock lens, reflex sights) writes the light and leaves other pixels as they were.
float4 reticle_atlas_core_out(float3 emission)
{
	return float4(emission, any(emission > 0.0) ? 1.0 : 0.0);
}

float4 reticle_atlas_glow_out(ReticleAtlas a, float3 emission)
{
	return float4(emission * a.glow_strength, any(emission > 0.0) ? 1.0 : 0.0);
}

// The glow source of an emission pass whose shader returns the core (scope_custom_reticle.h); the pass's
// wrapper writes it as its second output
static float4 reticle_atlas_glow_source = 0;

// Reflective paint: albedo times the luminance of the light behind the reticle, never emissive.
float3 reticle_atlas_paint(ReticleMarks m, float background_luminance)
{
	return m.paint > 0.0 ? m.paint_rgb / m.paint * background_luminance : 0.0;
}

bool reticle_atlas_emission_external()
{
	return reticle_atlas_control.x > 0.5;
}

#endif
