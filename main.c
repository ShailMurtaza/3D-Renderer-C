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
  char fps_text[32] = "";

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
      snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", current_fps);
      fps_frame_count = 0;
      fps_timer = now;
    }

    if (fps_text[0]) {
      SDL_Color white = {255, 255, 255, 255};
      SDL_Color black = {0, 0, 0, 255};

      TTF_SetFontOutline(font, 2);
      SDL_Surface *surf = TTF_RenderText_Blended(font, fps_text, 0, white);
      if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (tex) {
          SDL_FRect dst = {10.0f, 10.0f, (float)surf->w, (float)surf->h};
          SDL_RenderTexture(renderer, tex, NULL, &dst);
          SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
      }

      TTF_SetFontOutline(font, 0);
      surf = TTF_RenderText_Blended(font, fps_text, 0, black);
      if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (tex) {
          SDL_FRect dst = {10.0f, 10.0f, (float)surf->w, (float)surf->h};
          SDL_RenderTexture(renderer, tex, NULL, &dst);
          SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
      }
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
