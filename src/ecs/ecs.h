#pragma once
#include "../core/types.h"
#include <stdbool.h>

#define ECS_MAX_ENTITIES   65536
#define ECS_MAX_COMPONENTS 64
#define ECS_NULL           ((Entity)UINT32_MAX)

typedef u32 Entity;
typedef u32 ComponentId;

#define ECS_ENTITY_IDX(e)       ((e) & 0xFFFFu)
#define ECS_ENTITY_GEN(e)       ((e) >> 16)
#define ECS_ENTITY_MAKE(i, g)   ((Entity)(((u32)(g) << 16) | ((u32)(i) & 0xFFFFu)))

typedef struct World World;

/* world */
World *ecs_world_create(void);
void   ecs_world_destroy(World *w);

/* component registration */
ComponentId ecs_register(World *w, usize component_size);

/* entity lifecycle */
Entity ecs_create(World *w);
void   ecs_destroy(World *w, Entity e);
bool   ecs_alive(const World *w, Entity e);

/* component operations */
void  *ecs_add(World *w, Entity e, ComponentId cid, const void *data);
void   ecs_remove(World *w, Entity e, ComponentId cid);
void  *ecs_get(World *w, Entity e, ComponentId cid);
bool   ecs_has(const World *w, Entity e, ComponentId cid);

/* iteration — returns pointer to packed dense component array, sets count */
void  *ecs_view(World *w, ComponentId cid, u32 *out_count);

/* convenience: get the entity at dense index i for a component */
Entity ecs_view_entity(World *w, ComponentId cid, u32 i);