#include "transformations.h"
#include <math.h>

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

void project(Vec4 *vertices, int count, float near_plane, float far_plane) {
  float fx = 1.0f / tanf(FOV_HALF_DEG * (float)M_PI / 180.0f);
  float fy = fx;
  float ar = (float)WIDTH / (float)HEIGHT;
  float za = far_plane / (far_plane - near_plane);
  float zb = -far_plane * near_plane / (far_plane - near_plane);

  for (int i = 0; i < count; i++) {
    Vec4 v = vertices[i];
    vertices[i].x = v.x * (fx / ar);
    vertices[i].y = v.y * fy;
    vertices[i].z = v.z * za + v.w * zb;
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
  if (count <= 0) return;

  float min_x = vertices[0].x, max_x = vertices[0].x;
  float min_y = vertices[0].y, max_y = vertices[0].y;
  float min_z = vertices[0].z, max_z = vertices[0].z;
  for (int i = 1; i < count; i++) {
    if (vertices[i].x < min_x) min_x = vertices[i].x;
    if (vertices[i].x > max_x) max_x = vertices[i].x;
    if (vertices[i].y < min_y) min_y = vertices[i].y;
    if (vertices[i].y > max_y) max_y = vertices[i].y;
    if (vertices[i].z < min_z) min_z = vertices[i].z;
    if (vertices[i].z > max_z) max_z = vertices[i].z;
  }

  /* Use the bounding-box center so the object is framed symmetrically,
   * regardless of how its vertices are distributed. */
  translate(vertices, count, -(min_x + max_x) / 2.0f,
            -(min_y + max_y) / 2.0f, -(min_z + max_z) / 2.0f);
}

float object_radius(const Vec4 *vertices, int count) {
  float r2 = 0.0f;
  for (int i = 0; i < count; i++) {
    float d2 = vertices[i].x * vertices[i].x +
               vertices[i].y * vertices[i].y +
               vertices[i].z * vertices[i].z;
    if (d2 > r2) r2 = d2;
  }
  return sqrtf(r2);
}

void fit_camera(float radius, float *cam_dist, float *near_plane,
                float *far_plane) {
  /* Keep a small gap around the object: radius / tan(half-fov) is the distance
   * at which a sphere of that radius exactly touches top and bottom of the
   * screen, so a little extra distance gives a "just behind" framing. */
  const float margin = 1.2f;
  float tan_half = tanf(FOV_HALF_DEG * (float)M_PI / 180.0f);

  float dist = radius / tan_half * margin;
  if (dist < 0.1f) dist = 0.1f; /* degenerate/empty mesh fallback */

  float near = (dist - radius) * 0.5f;
  if (near > 0.1f) near = 0.1f;
  if (near < 1e-4f) near = 1e-4f;

  float far = (dist + radius) * 1.5f + 1.0f;
  if (far < 1000.0f) far = 1000.0f;

  *cam_dist = dist;
  *near_plane = near;
  *far_plane = far;
}
