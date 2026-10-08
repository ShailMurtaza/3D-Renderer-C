#include "clip.h"

#define LEFT   0b000001
#define RIGHT  0b000010
#define BOTTOM 0b000100
#define TOP    0b001000
#define NEAR   0b010000
#define FAR    0b100000

static int compute_outcode(float x, float y, float z, float w)
{
    int outcode = 0;
    if (x < -w) outcode |= LEFT;
    else if (x > w) outcode |= RIGHT;
    if (y < -w) outcode |= BOTTOM;
    else if (y > w) outcode |= TOP;
    if (z < 0) outcode |= NEAR;
    else if (z > w) outcode |= FAR;
    return outcode;
}

int cohen_sutherland_clip(Vec4 p1, Vec4 p2, Vec4 *out1, Vec4 *out2)
{
    float x1 = p1.x, y1 = p1.y, z1 = p1.z, w1 = p1.w;
    float x2 = p2.x, y2 = p2.y, z2 = p2.z, w2 = p2.w;

    int outcode1 = compute_outcode(x1, y1, z1, w1);
    int outcode2 = compute_outcode(x2, y2, z2, w2);

    while (1) {
        if (outcode1 == 0 && outcode2 == 0) {
            out1->x = x1; out1->y = y1; out1->z = z1; out1->w = w1;
            out2->x = x2; out2->y = y2; out2->z = z2; out2->w = w2;
            return 1;
        }

        if (outcode1 & outcode2)
            return 0;

        int outcode_out;
        if (outcode1 & NEAR) outcode_out = outcode1;
        else if (outcode2 & NEAR) outcode_out = outcode2;
        else if (outcode1 != 0) outcode_out = outcode1;
        else outcode_out = outcode2;

        float x = 0, y = 0, z = 0, w = 0;

        if (outcode_out & LEFT) {
            float t = (-w1 - x1) / ((x2 - x1) + (w2 - w1));
            y = y1 + t * (y2 - y1);
            z = z1 + t * (z2 - z1);
            w = w1 + t * (w2 - w1);
            x = -w;
        } else if (outcode_out & RIGHT) {
            float t = (w1 - x1) / ((x2 - x1) - (w2 - w1));
            y = y1 + t * (y2 - y1);
            z = z1 + t * (z2 - z1);
            w = w1 + t * (w2 - w1);
            x = w;
        } else if (outcode_out & BOTTOM) {
            float t = (-w1 - y1) / ((y2 - y1) + (w2 - w1));
            x = x1 + t * (x2 - x1);
            z = z1 + t * (z2 - z1);
            w = w1 + t * (w2 - w1);
            y = -w;
        } else if (outcode_out & TOP) {
            float t = (w1 - y1) / ((y2 - y1) - (w2 - w1));
            x = x1 + t * (x2 - x1);
            z = z1 + t * (z2 - z1);
            w = w1 + t * (w2 - w1);
            y = w;
        } else if (outcode_out & NEAR) {
            float t = -z1 / (z2 - z1);
            x = x1 + t * (x2 - x1);
            y = y1 + t * (y2 - y1);
            w = w1 + t * (w2 - w1);
            z = 0;
        } else if (outcode_out & FAR) {
            float t = (w1 - z1) / ((z2 - z1) - (w2 - w1));
            x = x1 + t * (x2 - x1);
            y = y1 + t * (y2 - y1);
            w = w1 + t * (w2 - w1);
            z = w;
        }

        if (outcode_out == outcode1) {
            x1 = x; y1 = y; z1 = z; w1 = w;
            outcode1 = compute_outcode(x1, y1, z1, w1);
        } else {
            x2 = x; y2 = y; z2 = z; w2 = w;
            outcode2 = compute_outcode(x2, y2, z2, w2);
        }
    }
}
