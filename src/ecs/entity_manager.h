#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include "recs.h"
/*
  Entity Manager Section:

  This section handles the active Entity ID pool, returning IDs that are available for use as well as making IDs that
  were deleted ready for reuse.
*/

struct entity_manager {
  //stores list of all unused entity IDs.
  recs_entity *set_of_ids;
  uint32_t num_active_entities;
  uint32_t max_entities;
};


void entity_manager_init(struct entity_manager *em, uint8_t *buffer, uint32_t max_entities);


recs_entity entity_manager_add(struct entity_manager *em);

void entity_manager_remove_at_index(struct entity_manager *em, uint32_t active_entity_index);

void entity_manager_remove(struct entity_manager *em, recs_entity e);

#endif// ENTITY_MANAGER_H

