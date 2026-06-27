#pragma once
#include "ecs.h"

/* Flat struct of ECS function pointers passed to managed code on startup.
   Field order and types must match GwEcsApi in EcsBindings.cs exactly. */
typedef struct {
    World       *(*world_create)(void);
    void         (*world_destroy)(World *);
    ComponentId  (*register_component)(World *, usize);
    Entity       (*entity_create)(World *);
    void         (*entity_destroy)(World *, Entity);
    bool         (*entity_alive)(const World *, Entity);
    void        *(*component_add)(World *, Entity, ComponentId, const void *);
    void         (*component_remove)(World *, Entity, ComponentId);
    void        *(*component_get)(World *, Entity, ComponentId);
    bool         (*component_has)(const World *, Entity, ComponentId);
    void        *(*view)(World *, ComponentId, u32 *);
    Entity       (*view_entity)(World *, ComponentId, u32);
} GwEcsApi;

static inline GwEcsApi gw_ecs_api_get(void) {
    return (GwEcsApi){
        .world_create       = ecs_world_create,
        .world_destroy      = ecs_world_destroy,
        .register_component = ecs_register,
        .entity_create      = ecs_create,
        .entity_destroy     = ecs_destroy,
        .entity_alive       = ecs_alive,
        .component_add      = ecs_add,
        .component_remove   = ecs_remove,
        .component_get      = ecs_get,
        .component_has      = ecs_has,
        .view               = ecs_view,
        .view_entity        = ecs_view_entity,
    };
}