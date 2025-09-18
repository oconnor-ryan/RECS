#ifndef entity_manager_H
#define entity_manager_H


#include <stdint.h>

#include "recs.h"

struct entity_manager {
  //stores list of all unused entity IDs.
  recs_entity *set_of_ids;
  uint32_t max_entities;
  uint32_t num_active_entities;
};



void entity_manager_init(struct entity_manager *em, recs_entity *buffer, uint32_t max_entities);

recs_entity entity_manager_add(struct entity_manager *em);

void entity_manager_remove_at_index(struct entity_manager *em, uint32_t active_entity_index);

void entity_manager_remove(struct entity_manager *em, recs_entity e);




#endif// entity_manager_H
