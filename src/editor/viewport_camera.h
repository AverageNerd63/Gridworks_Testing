#pragma once
#include "../math/gw_math.h"
#include "../platform/input.h"
#include <stdbool.h>

typedef struct {
    Vec3 focus;
    f32  distance;
    f32  pitch;
    f32  yaw;
    f32  fov_y;
    f32  near_z;
    f32  far_z;
    f32  orbit_sensitivity;
    f32  pan_sensitivity;
    f32  zoom_sensitivity;
} ViewportCamera;

void viewport_camera_init(ViewportCamera *cam);
void viewport_camera_update(ViewportCamera *cam, const InputState *in,
                             f32 dt, bool viewport_hovered);
void viewport_camera_get_vp(const ViewportCamera *cam, f32 aspect,
                              Mat4 *out_view, Mat4 *out_proj);
Vec3 viewport_camera_position(const ViewportCamera *cam);