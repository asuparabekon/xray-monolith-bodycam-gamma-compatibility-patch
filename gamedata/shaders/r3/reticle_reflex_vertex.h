#ifndef RETICLE_REFLEX_VERTEX_H
#define RETICLE_REFLEX_VERTEX_H

// Matches the output of 3DSS models_reflex_reticle.vs.
struct vf
{
	float4 hpos : SV_Position;
	float2 tc0 : TEXCOORD0;
	float3 P : TEXCOORD1;
	float3 T : TEXCOORD2;
	float3 B : TEXCOORD3;
	float3 N : TEXCOORD4;
};

#endif
