# Benchmark Results

RECS suffers from many performance issues, especially for functions that retrieve components and iterate over entities.

When running `benchmark.c` with 1000 entities for 100 ticks using Valgrind, it generates the following profile data:

| Relative Cost | Num Calls | Function | Location |
|---------------|-----------|----------|----------|
| 99.8 | 100 |  system_collision | tools/benchmark.c |
| 51.4 | 50150000 | recs_ent_iter_next | src/ecs.c |
| 46.9 | 50150200 | recs_ent_iter_find | src/ecs.c |
| 22.9 | 50050100 | recs_entity_matches_component | src/entity.c |
| 21.7 | 100300000 | recs_entity_get_component | src/entity.c |
| 16.9 | 49950000 | system_collision_process_colliders | tools/benchmark.c |

Notice that `recs_ent_iter_next`, `recs_ent_iter_find`, `recs_entity_matches_component`, and `recs_entity_get_component` are called very frequently, and take up a lot of time relative to the rest of the program. While part of the blame is using a O(n^2) algorithm to check collisions between every pair of entities, I'm sure there's also a problem with RECS itself in terms of how it handles iterating over entities and retrieving components.

> Just as a test, I recreated the same simulation environment without using RECS by creating a simple object pool of entities, with each entity containing a position, velocity, and collider data. A active entity list was also added to track the list of currently active entities. This code used the same O(n^2) algorithm to check every pair of entities for every tick. After running this 2nd simulation with 1000 entities for 100 ticks, the time it took to run the simulation was about `350 milliseconds`, which is about 5 times faster than using the simulation that relied on RECS, which took `1750 milliseconds`.



For one, the `recs_entity_get_component()` function is NOT cached by the entity manager, which means that you should try avoiding fetching the same component from the same entity multiple times within a single system. 

For example, the `benchmark.c` file creates a basic simulation where entities have a position, velocity, and collider. We move these colliders at a randomized velocity and perform collision checking between these colliders, updating a variable after each collision. For simplicity, we use a naive O(n^2) algorithm to perform collision checks between every unique pair of entities every frame. 

Notice in the below code segment that `system_collision_process_colliders` re-retrieves the 1st entity's components every time a collision check between 2 entities occurs:

```c

static void system_collision_process_colliders(recs ecs, recs_entity e1, recs_entity e2) {
  struct component_position *p1 = (struct component_position*) recs_entity_get_component(ecs, e1, COMP_TYPE_POSITION);
  struct component_collision *c1 = recs_entity_get_component(ecs, e1, COMP_TYPE_COLLISION);
  struct component_collision *c2 = recs_entity_get_component(ecs, e2, COMP_TYPE_COLLISION);
  struct component_position *p2 = (struct component_position*) recs_entity_get_component(ecs, e2, COMP_TYPE_POSITION);

  // rest of collision checking code...
}

static void system_collision(recs ecs) {
  uint8_t mask[RECS_GET_BITMASK_SIZE(COMP_TYPE_COUNT, TAG_COUNT)];
  recs_bitmask_create(ecs, mask, RECS_BITMASK_CREATE_COMP_ARG(2, COMP_TYPE_COLLISION, COMP_TYPE_POSITION), 0, NULL);
  recs_ent_iter iter = recs_ent_iter_init_with_match(ecs, mask, RECS_ENT_MATCH_ALL);

  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);
    recs_ent_iter i2 = iter;

    while(recs_ent_iter_has_next(&i2)) {
      recs_entity e2 = recs_ent_iter_next(ecs, &i2);
      system_collision_process_colliders(ecs, p1, c1, p2, c2);
    }
  }
}
```

When I ran this with 1000 entities (with all having the position, velocity, and collision components) for 100 ticks, the simulation took about `2 seconds` to run. However, when I moved the `recs_entity_get_component()` calls such that the 1st entity's components only get retrieved once per `system_collision()` call, performance improved to `1.75` seconds.


```c

static void system_collision_process_colliders(
  recs ecs, 
  struct component_position *p1,
  struct component_collision *c1,
  struct component_position *p2,
  struct component_collision *c2
) {
   
  // rest of collision checking code...
}
static void system_collision(recs ecs) {
  uint8_t mask[RECS_GET_BITMASK_SIZE(COMP_TYPE_COUNT, TAG_COUNT)];
  recs_bitmask_create(ecs, mask, RECS_BITMASK_CREATE_COMP_ARG(2, COMP_TYPE_COLLISION, COMP_TYPE_POSITION), 0, NULL);
  recs_ent_iter iter = recs_ent_iter_init_with_match(ecs, mask, RECS_ENT_MATCH_ALL);

  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);

    //notice we only retrieve these components for the 1st entity once per iteration
    //rather than retrieving it once per entity pair.
    struct component_collision *c1 = recs_entity_get_component(ecs, e, COMP_TYPE_COLLISION);
    struct component_position *p1 = (struct component_position*) recs_entity_get_component(ecs, e, COMP_TYPE_POSITION);

    recs_ent_iter i2 = iter;

    while(recs_ent_iter_has_next(&i2)) {
      recs_entity e2 = recs_ent_iter_next(ecs, &i2);

      struct component_collision *c2 = recs_entity_get_component(ecs, e2, COMP_TYPE_COLLISION);
      struct component_position *p2 = (struct component_position*) recs_entity_get_component(ecs, e2, COMP_TYPE_POSITION);

      system_collision_process_colliders(ecs, p1, c1, p2, c2);

    }
  }
}
```