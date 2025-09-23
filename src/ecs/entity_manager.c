#include "ecs/entity_manager.h"

/*
  We first initialize a set with the IDs 1 through MAX_ENTITIES.
  From this point on, we cannot allow duplicates within this set, 
  so we need our public header functions to avoid allowing users to insert duplicates.

  In the below state, no entities are active
  V 
  1, 2, 3, 4

  In the below state, entities 1 and 2 are active
        V
  1, 2, 3, 4

  After removing entity 1, we swap last active element with removed element and decrement end pointer
     V
  2, 1, 3, 4

  When adding another entity, simply increment the end pointer:

        V
  2, 1, 3, 4

  When remove entity 1 again:

     V  
  2, 3, 1, 4

  This way, we don't need 2 stacks of the same size to store active/inactive 
  IDs.

  We just read the left side for active IDs, and the right side for inactive entities.


*/

void entity_manager_init(struct entity_manager *em, uint8_t *buffer, uint32_t max_entities) {
  em->num_active_entities = 0;
  em->max_entities = max_entities;
  em->set_of_ids = (recs_entity*)buffer;

  //add initial entity IDs to set. 
  for(recs_entity i = 0; i < em->max_entities; i++) {
    em->set_of_ids[i] = i;
  }

}


recs_entity entity_manager_add(struct entity_manager *em) {
  RECS_ASSERT(em->num_active_entities < em->max_entities);

  recs_entity e = em->set_of_ids[em->num_active_entities];
  em->num_active_entities++;

  return e;

}

void entity_manager_remove_at_index(struct entity_manager *em, uint32_t active_entity_index) {
  uint32_t i = active_entity_index;

  //swap last ACTIVE ID with removed ID.

  uint32_t temp = em->set_of_ids[i];
  em->set_of_ids[i] = em->set_of_ids[em->num_active_entities-1];
  em->set_of_ids[em->num_active_entities-1] = temp;

  em->num_active_entities--;
}

void entity_manager_remove(struct entity_manager *em, recs_entity e) {
  RECS_ASSERT(em->num_active_entities != 0);

  //this requires a search for the ID.
  for(uint32_t i = 0; i < em->num_active_entities; i++) {
    if(em->set_of_ids[i] == e) {
      //swap last ACTIVE ID with removed ID.
      entity_manager_remove_at_index(em, i);
      return;
    }
  }

}
