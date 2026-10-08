#include "transformations.h"
#include <math.h>

static const float NEAR = 0.1f;
static const float FAR = 1000.0f;

void rotate_x(Vec4 *vertices, int count, float angle_deg) {
  float angle = angle_deg * (float)M_PI / 180.0f;
  float c = cosf(angle), s = sinf(angle);
  for (int i = 0; i < count; i++) {
    float y = vertices[i].y;
    float z = vertices[i].z;
    vertices[i].y = y * c - z * s;
    vertices[i].z = y * s + z * c;
  }
}

void rotate_y(Vec4 *vertices, int count, float angle_deg) {
  float angle = angle_deg * (float)M_PI / 180.0f;
  float c = cosf(angle), s = sinf(angle);
  for (int i = 0; i < count; i++) {
    float x = vertices[i].x;
    float z = vertices[i].z;
    vertices[i].x = x * c - z * s;
    vertices[i].z = x * s + z * c;
  }
}

void rotate_z(Vec4 *vertices, int count, float angle_deg) {
  float angle = angle_deg * (float)M_PI / 180.0f;
  float c = cosf(angle), s = sinf(angle);
  for (int i = 0; i < count; i++) {
    float x = vertices[i].x;
    float y = vertices[i].y;
    vertices[i].x = x * c - y * s;
    vertices[i].y = x * s + y * c;
  }
}

void translate(Vec4 *vertices, int count, float tx, float ty, float tz) {
  for (int i = 0; i < count; i++) {
    vertices[i].x += tx * vertices[i].w;
    vertices[i].y += ty * vertices[i].w;
    vertices[i].z += tz * vertices[i].w;
  }
}

void project(Vec4 *vertices, int count) {
  float fx = 1.0f / tanf(15.0f * (float)M_PI / 180.0f);
  float fy = fx;
  float ar = (float)WIDTH / (float)HEIGHT;

  for (int i = 0; i < count; i++) {
    Vec4 v = vertices[i];
    vertices[i].x = v.x * (fx / ar);
    vertices[i].y = v.y * fy;
    vertices[i].z =
        v.z * (FAR / (FAR - NEAR)) + v.w * (-FAR * NEAR / (FAR - NEAR));
    vertices[i].w = v.z;
  }
}

void perspective_divide(Vec4 *v) {
  v->x /= v->w;
  v->y /= v->w;
}

void map_to_screen(const Vec4 *v, float *sx, float *sy) {
  *sx = (v->x + 1.0f) / 2.0f * WIDTH;
  *sy = (1.0f - v->y) / 2.0f * HEIGHT;
}

void center_object(Vec4 *vertices, int count) {
  float cx = 0, cy = 0, cz = 0;
  for (int i = 0; i < count; i++) {
    cx += vertices[i].x;
    cy += vertices[i].y;
    cz += vertices[i].z;
  }
  cx /= count;
  cy /= count;
  cz /= count;
  translate(vertices, count, -cx, -cy, -cz);
}
