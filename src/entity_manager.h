#ifndef entity_manager_H
#define entity_manager_H


#include <stdint.h>


typedef uint32_t entity;

struct entity_manager {
  //stores list of all unused entity IDs.
  entity *set_of_ids;
  uint32_t max_entities;
  uint32_t num_active_entities;
};



int entity_manager_init(struct entity_manager *em, uint32_t max_entities);
void entity_manager_free(struct entity_manager *em);

entity entity_manager_add(struct entity_manager *em);

void entity_manager_remove_at_index(struct entity_manager *em, uint32_t active_entity_index);

void entity_manager_remove(struct entity_manager *em, entity e);




#endif// entity_manager_H
