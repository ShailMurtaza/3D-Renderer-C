#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clip.h"
#include "events.h"
#include "obj_loader.h"
#include "transformations.h"

static float cam_x = 0.0f, cam_y = 0.0f, cam_z = -150.0f;

static void draw_edge(SDL_Renderer *renderer, Vec4 p1, Vec4 p2) {
  perspective_divide(&p1);
  perspective_divide(&p2);

  float sx1, sy1, sx2, sy2;
  map_to_screen(&p1, &sx1, &sy1);
  map_to_screen(&p2, &sx2, &sy2);

  SDL_RenderLine(renderer, sx1, sy1, sx2, sy2);
}

/* Render a line of text into a texture. Returns NULL on failure. */
static SDL_Texture *make_text(SDL_Renderer *renderer, TTF_Font *font,
                              const char *text, SDL_Color color, float *w,
                              float *h) {
  SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, color);
  if (!surf) return NULL;

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
  *w = (float)surf->w;
  *h = (float)surf->h;
  SDL_DestroySurface(surf);
  return tex;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "No obj file provided!\n");
    return 1;
  }
  const char *obj_path = argv[1];

  Mesh *mesh = load_obj(obj_path);
  if (!mesh) {
    fprintf(stderr, "Failed to load OBJ file: %s\n", obj_path);
    return 1;
  }

  center_object(mesh->vertices, mesh->vertex_count);

  /* Place the camera just behind the loaded object and pick clip planes that
   * enclose it, so every model is framed consistently regardless of scale. */
  float radius = object_radius(mesh->vertices, mesh->vertex_count);
  float cam_dist, near_plane, far_plane;
  fit_camera(radius, &cam_dist, &near_plane, &far_plane);
  cam_z = -cam_dist;

  Vec4 *working = malloc(mesh->vertex_count * sizeof(Vec4));
  if (!working) {
    free_mesh(mesh);
    return 1;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    free(working);
    free_mesh(mesh);
    return 1;
  }

  if (!TTF_Init()) {
    fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
    SDL_Quit();
    free(working);
    free_mesh(mesh);
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("3D Renderer", WIDTH, HEIGHT, 0);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    TTF_Quit();
    SDL_Quit();
    free(working);
    free_mesh(mesh);
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    free(working);
    free_mesh(mesh);
    return 1;
  }

  TTF_Font *font = TTF_OpenFont("/usr/share/fonts/noto/NotoSans-Bold.ttf", 14);
  if (!font) {
    fprintf(stderr, "TTF_OpenFont failed: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    free(working);
    free_mesh(mesh);
    return 1;
  }

  TransformState state = {0};

  Uint64 prev_ticks = SDL_GetTicks();
  int running = 1;

  Uint64 fps_timer = 0;
  int fps_frame_count = 0;
  float current_fps = 0.0f;

  const char *model_name = strrchr(obj_path, '/');
  model_name = model_name ? model_name + 1 : obj_path;

  while (running) {
    Uint64 now = SDL_GetTicks();
    float dt = (now - prev_ticks) / 1000.0f;
    prev_ticks = now;

    running = process_events(&state, dt);

    memcpy(working, mesh->vertices, mesh->vertex_count * sizeof(Vec4));

    rotate_x(working, mesh->vertex_count, state.rot_x);
    rotate_y(working, mesh->vertex_count, state.rot_y);
    rotate_z(working, mesh->vertex_count, state.rot_z);
    translate(working, mesh->vertex_count, state.tx, state.ty, state.tz);
    translate(working, mesh->vertex_count, -cam_x, -cam_y, -cam_z);
    project(working, mesh->vertex_count, near_plane, far_plane);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int i = 0; i < mesh->edge_count; i++) {
      Vec4 p1 = working[mesh->edges[i].a];
      Vec4 p2 = working[mesh->edges[i].b];
      Vec4 c1, c2;
      if (cohen_sutherland_clip(p1, p2, &c1, &c2))
        draw_edge(renderer, c1, c2);
    }

    fps_frame_count++;
    if (now - fps_timer >= 1000) {
      current_fps = fps_frame_count * 1000.0f / (now - fps_timer);
      fps_frame_count = 0;
      fps_timer = now;
    }

    char hud[8][128];
    int hud_lines = 0;
    snprintf(hud[hud_lines++], sizeof(hud[0]), "FPS: %.1f", current_fps);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Model: %s", model_name);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Vertices: %d   Edges: %d",
             mesh->vertex_count, mesh->edge_count);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Camera dist: %.2f", cam_dist);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Position: (%.2f, %.2f, %.2f)",
             state.tx, state.ty, state.tz);
    snprintf(hud[hud_lines++], sizeof(hud[0]),
             "Rotation: (%.1f, %.1f, %.1f) deg", state.rot_x, state.rot_y,
             state.rot_z);
    snprintf(hud[hud_lines++], sizeof(hud[0]),
             "WASD/QE move   IJKL/UO rotate   Wheel zoom");

    /* Draw the HUD on a translucent panel so the white text stays readable
     * over the wireframe without needing an outline. */
    SDL_Color white = {255, 255, 255, 255};
    const float pad = 10.0f;
    const float line_h = (float)TTF_GetFontHeight(font) + 2.0f;

    float text_w = 0.0f;
    for (int i = 0; i < hud_lines; i++) {
      int w = 0, h = 0;
      TTF_GetStringSize(font, hud[i], 0, &w, &h);
      if ((float)w > text_w) text_w = (float)w;
    }

    SDL_FRect panel = {8.0f, 8.0f, text_w + pad * 2.0f,
                       (float)hud_lines * line_h + pad * 2.0f};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
    SDL_RenderRect(renderer, &panel);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    for (int i = 0; i < hud_lines; i++) {
      float w, h;
      SDL_Texture *tex = make_text(renderer, font, hud[i], white, &w, &h);
      if (!tex) continue;
      SDL_FRect dst = {panel.x + pad, panel.y + pad + (float)i * line_h, w, h};
      SDL_RenderTexture(renderer, tex, NULL, &dst);
      SDL_DestroyTexture(tex);
    }

    SDL_RenderPresent(renderer);
  }

  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  free(working);
  free_mesh(mesh);
  return 0;
}
