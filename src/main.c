#include "core/logger.h"
#include "platform/window.h"
#include "platform/input.h"
#include "platform/timing.h"
#include "renderer/renderer.h"

#include "ecs/ecs.h"

int main(void) {
    logger_init();
    LOG_INFO("Gridworks starting...");

    {
    World *world = ecs_world_create();

    typedef struct { f32 x, y, z; } Position;
    typedef struct { f32 x, y, z; } Velocity;

    ComponentId pos_id = ecs_register(world, sizeof(Position));
    ComponentId vel_id = ecs_register(world, sizeof(Velocity));

    Entity e1 = ecs_create(world);
    Entity e2 = ecs_create(world);

    ecs_add(world, e1, pos_id, &(Position){1.0f, 2.0f, 3.0f});
    ecs_add(world, e1, vel_id, &(Velocity){0.1f, 0.0f, 0.0f});
    ecs_add(world, e2, pos_id, &(Position){4.0f, 5.0f, 6.0f});

    /* iterate all positions */
    u32 count;
    Position *positions = ecs_view(world, pos_id, &count);
    for (u32 i = 0; i < count; i++)
        LOG_INFO("[ecs] entity %u pos (%.1f, %.1f, %.1f)",
                 ECS_ENTITY_IDX(ecs_view_entity(world, pos_id, i)),
                 positions[i].x, positions[i].y, positions[i].z);

    ecs_destroy(world, e1);
    LOG_INFO("[ecs] e1 alive: %d  e2 alive: %d", ecs_alive(world, e1), ecs_alive(world, e2));

    ecs_world_destroy(world);
    }


    InputState input;
    input_init(&input);

    GwWindow window = { .input = &input };
    if (!gw_window_create(&window, "Gridworks", 1280, 720))
        return 1;

    GwRendererDesc rdesc = {
        .window  = &window,
        .backend = GW_RENDERER_BACKEND_VULKAN,
    };
    if (!gw_renderer_init(&rdesc))
        return 1;

    Timer timer;
    timer_init(&timer);

    i32 prev_w = window.width, prev_h = window.height;
    while (window.running) {
    gw_window_poll(&window);
    timer_tick(&timer);
    if (input_key_pressed(&input, GW_KEY_ESCAPE)) window.running = false;
    if (window.width != prev_w || window.height != prev_h) {
        gw_renderer_on_resize((u32)window.width, (u32)window.height);
        prev_w = window.width;
        prev_h = window.height;
    }
    if (!gw_renderer_begin_frame()) continue;
    gw_renderer_end_frame();
}

    gw_renderer_wait_idle();
    gw_renderer_shutdown();
    LOG_INFO("Shutdown clean.");
    gw_window_destroy(&window);
    logger_shutdown();
    return 0;
}