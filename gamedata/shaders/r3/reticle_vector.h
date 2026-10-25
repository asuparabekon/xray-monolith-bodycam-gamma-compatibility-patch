// Vector reticles: reticle atlas format 12 (RGBA8, one mip). The reticle is drawn from its SVG's filled shapes as
// quadratic curves: each pixel gets the exact area of each shape inside it, so edges are exact at any
// magnification and lines and dots far below a pixel keep their true darkness. Parts in the drawing's "sfp" layer
// keep their size when the sight zooms, parts in its "dial" layer keep their size and turn about the centre with
// the zoom (ReticleView.dial_angle); the rest is in the sight's own focal plane (reticle_atlas_view). Illumination and emission:
// reticle_atlas.h. Written by the Bodycam engine's src/Layers/xrRender/reticle_vector (its README: drawing rules).
//
// Header, row 0, bytes:
//   0-7   'R','E','T','I','C','L','E', 12 (the format)
//   8-11  log2 width, log2 height, unit (1 MOA, 2 mrad), flags (RATLAS_FLAG_*)
//   12-15 u16 piece count, u16 curve count
//   16-19 u16 grid list entries, mask log2 width, mask log2 height (0 = no mask)
//   20-23 u16 mask first row, mask mean * 255, 0
//   24-27 u16 byte sum of the settings record, u16 byte sum of bytes 0-23
// Descriptor stream: texel k at (k % 128, 1 + k / 128), in this order:
//   The settings record, 36 bytes: 0-1 u16 calibrated magnification * 100, 2-3 u16 radius * 16 (coordinates are
//     steps of radius / 32767), 4 Mark Switch marks, 5 colours (0 the sight's Mark Switch colour, 1 the drawing's),
//     6-7 u16 legacy span * 8 (0 = true angles), 8-9 u16 luminance * 16, 10 glow strength * 32, 11 glow size * 32,
//     12 brightness steps, 13-20 each step * 255, hologram 21 softness * 32, 22 level * 100, 23 ghost level * 255,
//     24-25 s8 ghost offset * 8, 26 fringe * 100, 27 flicker * 255, mask 28 depth * 255, 29-30 u16 size (units per
//     repeat) * 16, 31 shift with the eye (units per mm) * 100, 32 grain changes per second * 16 (0 = still),
//     33 fibre (lit core per sky luminance) * 32, 34-35 u16 tritium (lit core, scene luminance) * 256.
//   The pieces, five texels each: role (RATLAS_ROLE_*: illum, paint, glow, light), flags (bit 0 keeps its
//     size, bit 1 turns as the dial (bit 0 set too), bits 4-7 the Mark Switch mark, 0 = every mark), r, g, b, 0, u16 group, u16 first curve, u16 curve
//     count, s16 bounds min x, min y, max x, max y. A piece is a drawn shape cut to one grid cell, its contours oriented so the filled side is on the
//     left; the pieces of a shape follow each other with one group number.
//   The curves, three texels each (start, control, end), a point per texel as s16 x, y in reticle units (x right,
//     y down).
//   The grid: 32 x 32 cells over [-radius, radius] in each direction, row by row, a texel each: u16 first entry of
//     the cell's list, u16 count; then the lists, a u16 per piece in ascending order: its index in the low 12 bits,
//     its Mark Switch mark in the top 4 (a hidden pattern is skipped unread). A cell lists every piece whose
//     bounds come within one cell size of it.
// The mask (holographic grain) is grey in R, one texel per pixel, from its first row.
#ifndef RETICLE_VECTOR_H
#define RETICLE_VECTOR_H

#define RVEC_VERSION 12
#define RVEC_SEGMENTS 4
#define RVEC_GRID 32
#define RVEC_PIECE_BASE RATLAS_RECORD_TEXELS // descriptor stream texel of the first piece, after the settings record
#define RVEC_PIECE_TEXELS 5

// a texel's two signed 16-bit coordinates
float2 rvec_coordinates(uint4 v)
{
	const uint2 c = uint2(v.x | (v.y << 8), v.z | (v.w << 8));
	return float2(c) - float2(c >= 32768) * 65536.0;
}

uint rvec_curve_base(ReticleAtlas a)
{
	return RVEC_PIECE_BASE + RVEC_PIECE_TEXELS * a.piece_count;
}

uint rvec_grid_base(ReticleAtlas a)
{
	return rvec_curve_base(a) + 3 * a.curve_count;
}

// a's header checks and fields, from its texels; a.recognised is set
ReticleAtlas reticle_vector_open(Texture2D t, ReticleAtlas a, uint w, uint h)
{
	uint4 head[7];
	uint header_sum = 0;
	[unroll] for (uint i = 0; i < 7; ++i)
	{
		head[i] = ratlas_texel(t, i, 0);
		if (i < 6)
			header_sum += head[i].x + head[i].y + head[i].z + head[i].w;
	}
	const uint4 t2 = head[2], t3 = head[3], t4 = head[4], t5 = head[5], t6 = head[6];
	a.dims = float2(w, h);
	a.unit = t2.z;
	a.flags = t2.w;
	a.piece_count = t3.x | (t3.y << 8);
	a.curve_count = t3.z | (t3.w << 8);
	a.list_entries = t4.x | (t4.y << 8);
	a.mask_size = t4.z > 0 ? uint2(1u << t4.z, 1u << t4.w) : uint2(0, 0);
	a.mask_row = t5.x | (t5.y << 8);
	a.mask_mean = t5.z / 255.0;
	bool ok = (1u << t2.x) == w && (1u << t2.y) == h && w >= RATLAS_STREAM_WIDTH && (a.unit == 1 || a.unit == 2)
		&& (a.flags & (RATLAS_FLAG_FFP_PARTS | RATLAS_FLAG_SFP_PARTS | RATLAS_FLAG_DIAL_PARTS)) != 0 && a.piece_count >= 1 && a.piece_count <= 4095
		&& t6.z == (header_sum & 255) && t6.w == ((header_sum >> 8) & 255);
	if (!ok)
		return a;

	// the settings record, loaded once: its checksum, then every field from the same texels
	uint4 rec[RATLAS_RECORD_TEXELS];
	uint record_sum = 0;
	[unroll] for (uint k = 0; k < RATLAS_RECORD_TEXELS; ++k)
	{
		rec[k] = ratlas_stream(t, k);
		record_sum += rec[k].x + rec[k].y + rec[k].z + rec[k].w;
	}
	ratlas_read_record(rec, a);
	const uint stream_end = rvec_grid_base(a) + RVEC_GRID * RVEC_GRID + (a.list_entries + 1) / 2;
	const uint stream_rows = 1 + (stream_end + RATLAS_STREAM_WIDTH - 1) / RATLAS_STREAM_WIDTH;
	a.valid = t6.x == (record_sum & 255) && t6.y == ((record_sum >> 8) & 255)
		&& a.mark_count >= 1 && a.mark_count <= 15 && ratlas_record_byte(rec, 5) <= 1 && a.coord_scale > 0.0
		&& stream_rows <= min(h, a.mask_size.x > 0 ? a.mask_row : h)
		&& (a.mask_size.x == 0 || (a.mask_size.x <= w && a.mask_row + a.mask_size.y <= h));
	return a;
}

// x(t), y(t) of the quadratic (A, B, C)
float2 rvec_bezier(float2 A, float2 B, float2 C, float s)
{
	return A + (2.0 * (B - A) + (A - 2.0 * B + C) * s) * s;
}

// Mean of clamp(x, 0, 1) as x runs linearly from x0 to x1
float rvec_mean_clamp(float x0, float x1)
{
	if (abs(x1 - x0) < 1e-5)
		return saturate(0.5 * (x0 + x1));
	const float2 x = float2(x0, x1);
	const float2 g = x <= 0.0 ? 0.0 : (x >= 1.0 ? x - 0.5 : 0.5 * x * x);
	return (g.y - g.x) / (x1 - x0);
}

// A straight edge's share of the unit pixel square: the integral of (1 - clamp(x, 0, 1)) dy over the part of the
// edge with y in [0, 1]. Summed over a closed outline it is the area of the outline inside the square.
float rvec_edge(float2 a, float2 b)
{
	const float ya = saturate(a.y), yb = saturate(b.y);
	if (ya == yb)
		return 0.0;
	const float k = (b.x - a.x) / (b.y - a.y);
	return (yb - ya) * (1.0 - rvec_mean_clamp(a.x + k * (ya - a.y), a.x + k * (yb - a.y)));
}

// The parameter in [t0, t1] where y reaches `target` on a piece of the curve that is monotone in y
float rvec_solve(float ay, float by, float cy, float target, float t0, float t1)
{
	const float c = cy - target;
	float root;
	if (abs(ay) < 1e-9 * max(abs(by), 1e-20))
		root = -c / by;
	else
	{
		const float q = -0.5 * (by + (by >= 0.0 ? 1.0 : -1.0) * sqrt(max(by * by - 4.0 * ay * c, 0.0)));
		const float r1 = q / ay;
		const float r2 = q != 0.0 ? c / q : r1;
		root = (r1 >= t0 - 1e-4 && r1 <= t1 + 1e-4) ? r1 : r2;
	}
	return clamp(root, t0, t1);
}

// The share of a y-monotone piece [t0, t1]: only its part inside the pixel's rows counts, flattened there
float rvec_piece(float2 A, float2 B, float2 C, float t0, float t1)
{
	const float ay = A.y - 2.0 * B.y + C.y, by = 2.0 * (B.y - A.y);
	const float y0 = rvec_bezier(A, B, C, t0).y, y1 = rvec_bezier(A, B, C, t1).y;
	if (y0 == y1 || max(y0, y1) <= 0.0 || min(y0, y1) >= 1.0)
		return 0.0;
	float ta = t0, tb = t1;
	if (y0 < 0.0 || y0 > 1.0)
		ta = rvec_solve(ay, by, A.y, y0 < 0.0 ? 0.0 : 1.0, t0, t1);
	if (y1 < 0.0 || y1 > 1.0)
		tb = rvec_solve(ay, by, A.y, y1 < 0.0 ? 0.0 : 1.0, t0, t1);
	float area = 0.0;
	float2 previous = rvec_bezier(A, B, C, ta);
	[unroll] for (uint s = 1; s <= RVEC_SEGMENTS; ++s)
	{
		const float2 next = rvec_bezier(A, B, C, lerp(ta, tb, float(s) / RVEC_SEGMENTS));
		area += rvec_edge(previous, next);
		previous = next;
	}
	return area;
}

// Area of the shape inside the pixel: its curves in pixel space (the pixel is the unit square), each split at
// its turning point in y
float rvec_shape_coverage(Texture2D t, ReticleAtlas a, uint first, uint count, float2 p, float2x2 to_pixel)
{
	float area = 0.0;
	[loop] for (uint c = first; c < first + count; ++c)
	{
		const uint k = rvec_curve_base(a) + 3 * c;
		const float2 A = mul(to_pixel, rvec_coordinates(ratlas_stream(t, k)) * a.coord_scale - p) + 0.5;
		const float2 B = mul(to_pixel, rvec_coordinates(ratlas_stream(t, k + 1)) * a.coord_scale - p) + 0.5;
		const float2 C = mul(to_pixel, rvec_coordinates(ratlas_stream(t, k + 2)) * a.coord_scale - p) + 0.5;
		if (max(A.y, max(B.y, C.y)) <= 0.0 || min(A.y, min(B.y, C.y)) >= 1.0)
			continue;
		const float ay = A.y - 2.0 * B.y + C.y;
		const float turn = abs(ay) > 1e-12 ? (A.y - B.y) / ay : -1.0;
		if (turn > 0.0 && turn < 1.0)
			area += rvec_piece(A, B, C, 0.0, turn) + rvec_piece(A, B, C, turn, 1.0);
		else
			area += rvec_piece(A, B, C, 0.0, 1.0);
	}
	return abs(area);
}

struct RvecPiece
{
	uint role, flags, group;
	float3 colour;
};

// A piece's share of the pixel and its record; -1 when the pixel is out of its reach (the record is not read),
// 0 when it is hidden, in the other focal plane or invalid
float rvec_piece_coverage(Texture2D t, ReticleAtlas a, uint i, float2 p, float2 reach, float2x2 to_pixel, uint pattern,
	uint plane, out RvecPiece piece)
{
	piece = (RvecPiece)0;
	const uint k = RVEC_PIECE_BASE + RVEC_PIECE_TEXELS * i;
	const float2 lo = rvec_coordinates(ratlas_stream(t, k + 3)) * a.coord_scale;
	const float2 hi = rvec_coordinates(ratlas_stream(t, k + 4)) * a.coord_scale;
	if (any(p + reach < lo) || any(p - reach > hi))
		return -1.0;
	const uint4 s0 = ratlas_stream(t, k), s1 = ratlas_stream(t, k + 1), s2 = ratlas_stream(t, k + 2);
	piece.role = s0.x;
	piece.flags = s0.y;
	piece.colour = float3(s0.zw, s1.x) / 255.0;
	piece.group = s1.z | (s1.w << 8);
	const uint mark = piece.flags >> 4, first = s2.x | (s2.y << 8), count = s2.z | (s2.w << 8);
	const uint piece_plane = (piece.flags & 1) + ((piece.flags >> 1) & piece.flags & 1); // 0 ffp, 1 sfp, 2 dial
	if (!((mark == 0 || mark == pattern) && piece_plane == plane && piece.role >= RATLAS_ROLE_ILLUM
		&& piece.role <= RATLAS_ROLE_LIGHT && first + count <= a.curve_count))
		return 0.0;
	return rvec_shape_coverage(t, a, first, count, p, to_pixel);
}

// The light colour of a lit shape: its own hue with the drawing's colours, else white (the sight's hue is
// applied with the illumination)
float3 rvec_light(ReticleAtlas a, float3 colour)
{
	return a.drawing_colours ? ratlas_hue(colour) : float3(1, 1, 1);
}

// A drawn shape's coverage (the sum of its pieces), added to its role's marks: coverage of separate shapes
// combines as 1 - (1 - a)(1 - b), per colour channel for light
void rvec_add_shape(ReticleAtlas a, RvecPiece piece, float coverage, float gamma, inout ReticleMarks m)
{
	if (coverage <= 0.0)
		return;
	const float cov = pow(saturate(coverage), gamma);
	if (piece.role == RATLAS_ROLE_ILLUM)
	{
		m.illum = 1.0 - (1.0 - m.illum) * (1.0 - cov);
		m.illum_rgb = 1.0 - (1.0 - m.illum_rgb) * (1.0 - cov * rvec_light(a, piece.colour));
	}
	else if (piece.role == RATLAS_ROLE_LIGHT)
		m.illum_rgb = 1.0 - (1.0 - m.illum_rgb) * (1.0 - cov * rvec_light(a, piece.colour));
	else
	{
		m.paint = 1.0 - (1.0 - m.paint) * (1.0 - cov);
		m.paint_rgb += piece.colour * cov;
		if (piece.role == RATLAS_ROLE_GLOW)
			m.glow_rgb = 1.0 - (1.0 - m.glow_rgb) * (1.0 - cov * rvec_light(a, piece.colour));
	}
}

// The shapes of one plane (0 the sight's own, 1 keeps its size, 2 the dial) at reticle angle p, jx/jy = reticle
// angle per screen pixel. While the box fits in a grid cell only the pieces listed for the pixel's cell are tested;
// a reticle so small that a pixel spans more than a cell tests every piece. Pieces of one drawn shape follow each
// other and share a group number: their coverage adds up before the shape joins the marks.
void rvec_marks_plane(Texture2D t, ReticleAtlas a, float2 p, float2 jx, float2 jy, uint pattern, uint plane,
	inout ReticleMarks m)
{
	const float gamma = 1.0 - 0.75 * reticle_atlas_line_weight();
	const float soften = max(ratlas_soften, 1.0);
	const float2x2 box = float2x2(jx.x, jy.x, jx.y, jy.y) * soften;
	const float det = box._11 * box._22 - box._12 * box._21;
	const float2 reach = 0.5 * soften * (abs(jx) + abs(jy));
	const float radius = a.coord_scale * 32767.0;
	if (abs(det) < 1e-20 || any(abs(p) > radius + reach))
		return;
	const float2x2 to_pixel = float2x2(box._22, -box._12, -box._21, box._11) / det;
	const float cell = 2.0 * radius / RVEC_GRID;
	const bool listed = all(reach <= cell);
	const uint lists = rvec_grid_base(a) + RVEC_GRID * RVEC_GRID;
	uint first = 0, count = a.piece_count;
	if (listed)
	{
		const uint2 c = uint2(clamp(floor((p + radius) / cell), 0.0, float(RVEC_GRID - 1)));
		const uint4 entry = ratlas_stream(t, rvec_grid_base(a) + c.y * RVEC_GRID + c.x);
		first = entry.x | (entry.y << 8);
		// a cell's range is outside the checksums: keep it inside the lists
		count = first <= a.list_entries ? min(entry.z | (entry.w << 8), a.list_entries - first) : 0;
	}
	RvecPiece shape = (RvecPiece)0;
	float shape_coverage = 0.0;
	bool open = false;
	[loop] for (uint j = 0; j < count; ++j)
	{
		uint i = j;
		if (listed)
		{
			const uint4 v = ratlas_stream(t, lists + (first + j) / 2);
			const uint entry = ((first + j) & 1) ? (v.z | (v.w << 8)) : (v.x | (v.y << 8));
			const uint mark = entry >> 12;
			if (mark != 0 && mark != pattern)
				continue;
			i = entry & 0xFFF;
		}
		if (i >= a.piece_count)
			continue;
		RvecPiece piece;
		const float c = rvec_piece_coverage(t, a, i, p, reach, to_pixel, pattern, plane, piece);
		if (c < 0.0)
			continue;
		if (open && piece.group != shape.group)
		{
			rvec_add_shape(a, shape, shape_coverage, gamma, m);
			shape_coverage = 0.0;
		}
		shape = piece;
		shape_coverage += c;
		open = true;
	}
	if (open)
		rvec_add_shape(a, shape, shape_coverage, gamma, m);
}

// The marks (reticle_atlas_marks): the shapes of Mark Switch mark `current`, each plane at its own place
ReticleMarks reticle_vector_marks(Texture2D t, ReticleAtlas a, ReticleView v, float current)
{
	ReticleMarks m = (ReticleMarks)0;
	const uint pattern = uint(max(current, 0.0)) % a.mark_count + 1;
	// the dial's place on its drawing: the sfp place turned back by the dial's angle (as 3DSS samples its 1P59 dial)
	const float c = cos(v.dial_angle), s = sin(v.dial_angle);
	const float2x2 turn = float2x2(c, -s, s, c);
	// one loop, so the pass is compiled once
	[loop] for (uint plane = 0; plane < 3; ++plane)
	{
		const uint flag = plane == 0 ? RATLAS_FLAG_FFP_PARTS : (plane == 1 ? RATLAS_FLAG_SFP_PARTS : RATLAS_FLAG_DIAL_PARTS);
		if (!(a.flags & flag))
			continue;
		const float2 p = plane == 0 ? v.ffp : (plane == 1 ? v.sfp : mul(turn, v.sfp));
		const float2 jx = plane == 0 ? v.ffp_dx : (plane == 1 ? v.sfp_dx : mul(turn, v.sfp_dx));
		const float2 jy = plane == 0 ? v.ffp_dy : (plane == 1 ? v.sfp_dy : mul(turn, v.sfp_dy));
		rvec_marks_plane(t, a, p, jx, jy, pattern, plane, m);
	}
	// an illuminated element is etched too: black paint under its light
	m.paint = 1.0 - (1.0 - m.paint) * (1.0 - m.illum);
	return m;
}

// The holographic mask at mask texel q, bilinear, repeating
float rvec_mask_sample(Texture2D t, ReticleAtlas a, float2 q)
{
	const float2 f = q - 0.5, base = floor(f), w = f - base;
	const int2 size = int2(a.mask_size);
	const int2 i0 = (int2(base) % size + size) % size, i1 = (i0 + 1) % size;
	const float v00 = t.Load(int3(i0.x, a.mask_row + i0.y, 0)).r, v10 = t.Load(int3(i1.x, a.mask_row + i0.y, 0)).r;
	const float v01 = t.Load(int3(i0.x, a.mask_row + i1.y, 0)).r, v11 = t.Load(int3(i1.x, a.mask_row + i1.y, 0)).r;
	return lerp(lerp(v00, v10, w.x), lerp(v01, v11, w.x), w.y);
}

// The mask averaged over the pixel's footprint at mask texel q: four taps spread over the pixel
float rvec_mask_mean(Texture2D t, ReticleAtlas a, float2 q, float2 u, float2 v)
{
	return 0.25 * (rvec_mask_sample(t, a, q + u) + rvec_mask_sample(t, a, q - u) + rvec_mask_sample(t, a, q + v)
		+ rvec_mask_sample(t, a, q - v));
}

// a pseudo-random offset into the mask for step `i` of an animated grain (repeating every 4096 steps keeps the hash
// precise however long the game runs)
float2 rvec_grain_offset(ReticleAtlas a, float i)
{
	i = fmod(i, 4096.0);
	return frac(sin(float2(i * 12.9898, i * 78.233)) * 43758.5453) * float2(a.mask_size);
}

// A hologram's grain: the author's mask, fixed in reticle angle and moving with the eye, relative to its mean so
// the average light is kept. Four taps spread over the pixel average what it covers, and grain the pixel cannot
// resolve fades out instead of crawling: from 4 to 10 mask texels per pixel, a mask grain being about 4 texels.
// An animated grain cross-fades to a new random place in the mask that many times a second.
// depth_scale = r__reticle_msdf_speckle.
float reticle_vector_grain(Texture2D t, ReticleAtlas a, float2 angle, float2 eye_mm, float2 jx, float2 jy,
	float depth_scale)
{
	const float depth = a.grain_depth * depth_scale;
	if (a.mask_size.x == 0 || a.mask_mean <= 0.0 || depth <= 0.0)
		return 1.0;
	const float2 texels = float2(a.mask_size) / a.grain_size;
	const float2 q = (angle + eye_mm * a.grain_shift) * texels;
	const float2 dx = jx * texels, dy = jy * texels;
	const float2 u = 0.25 * (dx + dy), v = 0.25 * (dx - dy);
	float mean;
	if (a.grain_rate <= 0.0)
		mean = rvec_mask_mean(t, a, q, u, v);
	else
	{
		const float s = timers.x * a.grain_rate, step_index = floor(s);
		mean = lerp(rvec_mask_mean(t, a, q + rvec_grain_offset(a, step_index), u, v),
			rvec_mask_mean(t, a, q + rvec_grain_offset(a, step_index + 1.0), u, v), smoothstep(0.0, 1.0, s - step_index));
	}
	const float footprint = max(length(dx), length(dy));
	const float grain = lerp(mean / a.mask_mean, 1.0, smoothstep(4.0, 10.0, footprint));
	return lerp(1.0, grain, saturate(depth));
}

#endif
