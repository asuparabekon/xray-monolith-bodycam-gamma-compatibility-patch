#include "common.h"
#include "skin.h"

struct scope_glass_vertex
{
	float4 hpos : SV_Position;
	float2 tc0 : TEXCOORD0;
	float3 position_world : TEXCOORD1;
	float3 tangent_world : TEXCOORD2;
	float3 binormal_world : TEXCOORD3;
	float3 normal_world : TEXCOORD4;
};

scope_glass_vertex scope_glass_main(v_model vertex)
{
	scope_glass_vertex result;
	result.hpos = mul(m_WVP, vertex.P);
	result.tc0 = vertex.tc.xy;
	result.position_world = mul(m_W, vertex.P).xyz;
	result.tangent_world = mul((float3x3)m_W, vertex.T.xyz);
	result.binormal_world = mul((float3x3)m_W, vertex.B.xyz);
	result.normal_world = mul((float3x3)m_W, vertex.N.xyz);
	return result;
}

#ifdef SKIN_NONE
scope_glass_vertex main(v_model vertex) { return scope_glass_main(vertex); }
#endif

#ifdef SKIN_0
scope_glass_vertex main(v_model_skinned_0 vertex) { return scope_glass_main(skinning_0(vertex)); }
#endif

#ifdef SKIN_1
scope_glass_vertex main(v_model_skinned_1 vertex) { return scope_glass_main(skinning_1(vertex)); }
#endif

#ifdef SKIN_2
scope_glass_vertex main(v_model_skinned_2 vertex) { return scope_glass_main(skinning_2(vertex)); }
#endif

#ifdef SKIN_3
scope_glass_vertex main(v_model_skinned_3 vertex) { return scope_glass_main(skinning_3(vertex)); }
#endif

#ifdef SKIN_4
scope_glass_vertex main(v_model_skinned_4 vertex) { return scope_glass_main(skinning_4(vertex)); }
#endif

