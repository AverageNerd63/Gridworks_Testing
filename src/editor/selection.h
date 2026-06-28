#pragma once
#include "../math/gw_math.h"
#include <stdbool.h>

#define SELECTION_NONE (-1)

typedef struct {
    int selected_id;
} SelectionState;

typedef struct {
    Vec3 origin;
    Vec3 dir;
} Ray;

void   selection_init(SelectionState *s);
void   selection_set(SelectionState *s, int id);
void   selection_clear(SelectionState *s);
bool   selection_has(const SelectionState *s);
int    selection_get(const SelectionState *s);

Ray    ray_from_screen(Mat4 inv_vp, f32 sx, f32 sy, f32 vw, f32 vh);
bool   ray_intersect_sphere(Ray r, Vec3 center, f32 radius, f32 *out_t);
bool   ray_intersect_aabb(Ray r, Vec3 mn, Vec3 mx, f32 *out_t);