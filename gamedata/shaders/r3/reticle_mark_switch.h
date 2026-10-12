// Mark Switch packs two signed dot offsets into the colour alpha channel.
#ifndef RETICLE_MARK_SWITCH_H
#define RETICLE_MARK_SWITCH_H

float2 reticle_mark_offset(float alpha)
{
    if (alpha <= 0.0)
        return float2(0.0, 0.0);

    float packed = floor(saturate(alpha) * 4198401.0 + 0.5) - 1.0;
    float x = floor(packed / 2049.0);
    float y = packed - x * 2049.0;
    return (float2(x, y) / 2048.0 * 2.0 - 1.0) * 0.01;
}

#endif
