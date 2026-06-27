#include "ecs.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>

/* ---- sparse set ------------------------------------------------------- */

typedef struct {
    u32    sparse[ECS_MAX_ENTITIES]; /* entity index → dense index          */
    Entity *dense_entities;          /* packed entity handles                */
    u8     *dense_data;              /* packed component data                */
    u32     count;
    usize   stride;                  /* sizeof one component                 */
} SparseSet;

static void ss_init(SparseSet *ss, usize stride) {
    memset(ss->sparse, 0xFF, sizeof(ss->sparse)); /* UINT32_MAX = absent */
    ss->dense_entities = NULL;
    ss->dense_data     = NULL;
    ss->count          = 0;
    ss->stride         = stride;
}

static void ss_free(SparseSet *ss) {
    free(ss->dense_entities);
    free(ss->dense_data);
    ss->dense_entities = NULL;
    ss->dense_data     = NULL;
    ss->count          = 0;
}

static void *ss_add(SparseSet *ss, u32 idx, const void *data) {
    if (ss->sparse[idx] != UINT32_MAX) return ss->dense_data + ss->sparse[idx] * ss->stride;

    u32 di = ss->count++;
    ss->dense_entities = realloc(ss->dense_entities, ss->count * sizeof(Entity));
    ss->dense_data     = realloc(ss->dense_data,     ss->count * ss->stride);

    ss->dense_entities[di] = (Entity)idx;
    ss->sparse[idx]        = di;

    void *slot = ss->dense_data + di * ss->stride;
    if (data) memcpy(slot, data, ss->stride);
    else      memset(slot, 0,    ss->stride);
    return slot;
}

static void ss_remove(SparseSet *ss, u32 idx) {
    if (ss->sparse[idx] == UINT32_MAX) return;

    u32 di       = ss->sparse[idx];
    u32 last     = ss->count - 1;
    u32 last_idx = ECS_ENTITY_IDX(ss->dense_entities[last]);

    /* swap with last */
    ss->dense_entities[di] = ss->dense_entities[last];
    memcpy(ss->dense_data + di * ss->stride,
           ss->dense_data + last * ss->stride,
           ss->stride);

    ss->sparse[last_idx] = di;
    ss->sparse[idx]      = UINT32_MAX;
    ss->count--;
}

static void *ss_get(const SparseSet *ss, u32 idx) {
    if (ss->sparse[idx] == UINT32_MAX) return NULL;
    return ss->dense_data + ss->sparse[idx] * ss->stride;
}

/* ---- world ------------------------------------------------------------ */

struct World {
    /* entity free list */
    u16  generations[ECS_MAX_ENTITIES]; /* per-slot generation counters      */
    u32  free_list[ECS_MAX_ENTITIES];   /* stack of free indices             */
    u32  free_head;                     /* top of free stack                 */
    u32  entity_count;

    /* component storage */
    SparseSet  sets[ECS_MAX_COMPONENTS];
    u32        component_count;
};

World *ecs_world_create(void) {
    World *w = calloc(1, sizeof(World));

    /* push all indices onto the free list */
    for (u32 i = 0; i < ECS_MAX_ENTITIES; i++)
        w->free_list[i] = ECS_MAX_ENTITIES - 1 - i; /* reverse so idx 0 is first */
    w->free_head = ECS_MAX_ENTITIES;

    LOG_INFO("[ecs] world created (max entities: %u, max components: %u)",
             ECS_MAX_ENTITIES, ECS_MAX_COMPONENTS);
    return w;
}

void ecs_world_destroy(World *w) {
    for (u32 i = 0; i < w->component_count; i++)
        ss_free(&w->sets[i]);
    free(w);
    LOG_INFO("[ecs] world destroyed");
}

/* ---- component registration ------------------------------------------ */

ComponentId ecs_register(World *w, usize component_size) {
    if (w->component_count >= ECS_MAX_COMPONENTS) {
        LOG_ERROR("[ecs] component limit reached (%u)", ECS_MAX_COMPONENTS);
        return UINT32_MAX;
    }
    ComponentId cid = w->component_count++;
    ss_init(&w->sets[cid], component_size);
    LOG_DEBUG("[ecs] registered component %u (size: %zu)", cid, component_size);
    return cid;
}

/* ---- entity lifecycle ------------------------------------------------- */

Entity ecs_create(World *w) {
    if (w->free_head == 0) {
        LOG_ERROR("[ecs] entity limit reached (%u)", ECS_MAX_ENTITIES);
        return ECS_NULL;
    }
    u32 idx = w->free_list[--w->free_head];
    u16 gen = w->generations[idx];
    w->entity_count++;
    return ECS_ENTITY_MAKE(idx, gen);
}

void ecs_destroy(World *w, Entity e) {
    if (!ecs_alive(w, e)) return;
    u32 idx = ECS_ENTITY_IDX(e);

    /* remove from all component sets */
    for (u32 i = 0; i < w->component_count; i++)
        ss_remove(&w->sets[i], idx);

    /* bump generation to invalidate old handles */
    w->generations[idx]++;
    w->free_list[w->free_head++] = idx;
    w->entity_count--;
}

bool ecs_alive(const World *w, Entity e) {
    if (e == ECS_NULL) return false;
    u32 idx = ECS_ENTITY_IDX(e);
    u16 gen = ECS_ENTITY_GEN(e);
    return w->generations[idx] == gen;
}

/* ---- component operations --------------------------------------------- */

void *ecs_add(World *w, Entity e, ComponentId cid, const void *data) {
    if (!ecs_alive(w, e) || cid >= w->component_count) return NULL;
    return ss_add(&w->sets[cid], ECS_ENTITY_IDX(e), data);
}

void ecs_remove(World *w, Entity e, ComponentId cid) {
    if (!ecs_alive(w, e) || cid >= w->component_count) return;
    ss_remove(&w->sets[cid], ECS_ENTITY_IDX(e));
}

void *ecs_get(World *w, Entity e, ComponentId cid) {
    if (!ecs_alive(w, e) || cid >= w->component_count) return NULL;
    return ss_get(&w->sets[cid], ECS_ENTITY_IDX(e));
}

bool ecs_has(const World *w, Entity e, ComponentId cid) {
    if (!ecs_alive(w, e) || cid >= w->component_count) return false;
    return w->sets[cid].sparse[ECS_ENTITY_IDX(e)] != UINT32_MAX;
}

void *ecs_view(World *w, ComponentId cid, u32 *out_count) {
    if (cid >= w->component_count) { *out_count = 0; return NULL; }
    *out_count = w->sets[cid].count;
    return w->sets[cid].dense_data;
}

Entity ecs_view_entity(World *w, ComponentId cid, u32 i) {
    if (cid >= w->component_count || i >= w->sets[cid].count) return ECS_NULL;
    return w->sets[cid].dense_entities[i];
}