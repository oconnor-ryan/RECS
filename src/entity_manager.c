#include "entity_manager.h"

/*
  In order to make it possible to add and remove entities during iteration, 
  we could use the following method:

  - If we store our active IDs as a list of recs_entity rather than just the ID, we could just skip over
  recently deleted active IDs during iteration, then call a function to permanently free those IDs to the inactive ID pool.
  As for inactive ids, we can "cheat" by giving them an unused version number. This way, 
  both the active and inactive ids are the same size and we can still use our method of filling
  holes left behind by removed ids.
*/
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
void entity_manager_init(struct entity_manager *em, uint8_t *id_buffer, uint8_t *version_buffer, uint32_t max_entities) {
  em->num_active_entities = 0;
  em->max_entities = max_entities;
  em->entity_pool = (recs_entity*)id_buffer;
  em->ent_versions_list = (uint32_t*) version_buffer;

  for(uint32_t i = 0; i < em->max_entities; i++) {
    //add initial entity IDs to set. 
    em->entity_pool[i] = RECS_ENT_FROM(i, 0); //note that version number is unused here, so any value is valid

    //set all versions to 0
    em->ent_versions_list[i] = 0;
  }

  

}


recs_entity entity_manager_add(struct entity_manager *em) {
  RECS_ASSERT(em->num_active_entities < em->max_entities);

  uint32_t id = RECS_ENT_ID(em->entity_pool[em->num_active_entities]);
  uint32_t version = em->ent_versions_list[id];

  //note that the active entities in the pool MUST HAVE VALID VERSION NUMBERS.
  //Thus, we update the version number here
  recs_entity e = RECS_ENT_FROM(id, version);
  em->entity_pool[em->num_active_entities] = e;


  em->num_active_entities++;


  return e;

}



void entity_manager_remove_at_index(struct entity_manager *em, uint32_t active_entity_index) {
  uint32_t i = active_entity_index;

  //swap last ACTIVE ID with removed ID.

  recs_entity removed = em->entity_pool[i];
  em->entity_pool[i] = em->entity_pool[em->num_active_entities-1];
  em->entity_pool[em->num_active_entities-1] = removed;

  em->num_active_entities--;

  
}

void entity_manager_remove(struct entity_manager *em, recs_entity e) {
  for(uint32_t i = 0; i < em->num_active_entities; i++) {
    if(em->entity_pool[i] == e) {
      entity_manager_remove_at_index(em, i);
      break;
    }
  }
}


