#include "common.h"
#include "skin.h"
#include "screenspace_mvectors.h"
struct scope_glass_deferred_vertex
{
 float4 hpos : SV_Position;
 float2 uv : TEXCOORD0;
 float3 world : TEXCOORD1;
 float3 tangent : TEXCOORD2;
 float3 bitangent : TEXCOORD3;
 float3 normal : TEXCOORD4;
 float4 current_clip : TEXCOORD5;
 float4 previous_clip : TEXCOORD6;
};
scope_glass_deferred_vertex scope_glass_deferred_main(v_model v)
{
 scope_glass_deferred_vertex o;
 o.current_clip=mul(m_WVP,v.P);
 o.previous_clip=mul(m_wvp_prev,v.P_prev);
 o.hpos=o.current_clip;
 o.hpos.xy=ssfx_taa_jitter(o.hpos);
 o.uv=v.tc.xy;
 o.world=mul(m_W,v.P).xyz;
 o.tangent=mul((float3x3)m_W,v.T.xyz);
 o.bitangent=mul((float3x3)m_W,v.B.xyz);
 o.normal=mul((float3x3)m_W,v.N.xyz);
 return o;
}
#ifdef SKIN_NONE
scope_glass_deferred_vertex main(v_model v){return scope_glass_deferred_main(v);}
#endif
#ifdef SKIN_0
scope_glass_deferred_vertex main(v_model_skinned_0 v){return scope_glass_deferred_main(skinning_0(v));}
#endif
#ifdef SKIN_1
scope_glass_deferred_vertex main(v_model_skinned_1 v){return scope_glass_deferred_main(skinning_1(v));}
#endif
#ifdef SKIN_2
scope_glass_deferred_vertex main(v_model_skinned_2 v){return scope_glass_deferred_main(skinning_2(v));}
#endif
#ifdef SKIN_3
scope_glass_deferred_vertex main(v_model_skinned_3 v){return scope_glass_deferred_main(skinning_3(v));}
#endif
#ifdef SKIN_4
scope_glass_deferred_vertex main(v_model_skinned_4 v){return scope_glass_deferred_main(skinning_4(v));}
#endif
