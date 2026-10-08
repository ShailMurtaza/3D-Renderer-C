#ifndef CLIP_H
#define CLIP_H

#include "transformations.h"

int cohen_sutherland_clip(Vec4 p1, Vec4 p2, Vec4 *out1, Vec4 *out2);

#endif
