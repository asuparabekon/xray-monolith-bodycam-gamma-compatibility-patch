// Encodes the glass roughness as the gloss value that the installed pseudo-PBR
// decodes back to it. Material IDs are quantised in the G-buffer.
#ifndef SCOPE_GLASS_PBR_MATERIAL_H
#define SCOPE_GLASS_PBR_MATERIAL_H
#include "pbr_settings.h"
// Bin 24 decodes to 1.032258: outside terrain(.91..99) and flora(.11..19).
// Input 1 would quantize to .989247 and accidentally select terrain.
static const float scope_glass_pbr_material_id=(24.5/31.0)*(4.0/3.0);
static const float scope_glass_pbr_decoded_id=(24.0/31.0)*(4.0/3.0);
float scope_glass_pbr_metal_threshold()
{
 return pow(METALNESS_THRESHOLD,exp2(max(0,scope_glass_pbr_decoded_id*4-.5)/4));
}
float scope_glass_pbr_roughness(float gloss,float glossControl)
{
 float threshold=scope_glass_pbr_metal_threshold();
 float metal=saturate((gloss-threshold*.1)/(threshold*1.8));
 float shaped=pow(saturate(gloss),ROUGHNESS_POW-metal*METAL_BOOST);
 return saturate(pow(lerp(ROUGHNESS_HIGH,ROUGHNESS_LOW,shaped),
     1/max(.001,1-glossControl)));
}
float scope_glass_pbr_gloss(float roughness,float glossControl)
{
 float target=saturate(roughness);
 if(target<=scope_glass_pbr_roughness(1,glossControl)) return 1;
 if(target>=scope_glass_pbr_roughness(0,glossControl)) return 0;
 // Exact inverse in the fully-metal region (the normal polished lens path).
 float shaped=saturate((ROUGHNESS_HIGH-pow(target,max(.001,1-glossControl)))/
     max(.0001,ROUGHNESS_HIGH-ROUGHNESS_LOW));
 float candidate=pow(shaped,1/max(.001,ROUGHNESS_POW-METAL_BOOST));
 if(candidate>=scope_glass_pbr_metal_threshold()*1.9) return candidate;
 // Below that gloss the stock decode also changes metalness; bisect.
 float lo=0,hi=1;
 [unroll] for(int i=0;i<12;++i)
 {
  float mid=(lo+hi)*.5;
  if(scope_glass_pbr_roughness(mid,glossControl)>target) lo=mid;
  else hi=mid;
 }
 return (lo+hi)*.5;
}
#endif
