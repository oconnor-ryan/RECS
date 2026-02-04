#include "types.h"
#include "bitmask.h"
#include "tags.h"


// START Internal functions




static inline recs_entity entity_manager_add(struct entity_manager *em) {
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

static void entity_manager_remove_at_index(struct entity_manager *em, uint32_t active_entity_index) {
  uint32_t i = active_entity_index;

  //swap last ACTIVE ID with removed ID.

  recs_entity removed = em->entity_pool[i];
  em->entity_pool[i] = em->entity_pool[em->num_active_entities-1];
  em->entity_pool[em->num_active_entities-1] = removed;

  em->num_active_entities--;

  
}

static void entity_manager_remove(struct entity_manager *em, recs_entity e) {
  for(uint32_t i = 0; i < em->num_active_entities; i++) {
    if(em->entity_pool[i] == e) {
      entity_manager_remove_at_index(em, i);
      break;
    }
  }
}


static void *component_pool_get(struct component_pool *ca, recs_entity e) {
  
  uint32_t component_index = ca->entity_to_comp[RECS_ENT_ID(e)];

  if(component_index == NO_COMP_ID) {
    return NULL;
  }
  return ca->buffer + (ca->component_size * component_index);
}


static void component_pool_add(struct component_pool *ca, recs_entity e, void *component) {
  RECS_ASSERT(ca->num_components < ca->max_components);

  uint32_t component_index = ca->num_components;

  memcpy(ca->buffer + (ca->component_size * component_index), component, ca->component_size);

  ca->comp_to_entity[component_index] = RECS_ENT_ID(e);
  ca->entity_to_comp[RECS_ENT_ID(e)] = component_index;

  ca->num_components++;

}

static void component_pool_remove(struct component_pool *ca, recs_entity e) {
  uint32_t component_index = ca->entity_to_comp[RECS_ENT_ID(e)];

  if(component_index == NO_COMP_ID) {
    return;
  }
  uint32_t last_component_index = ca->num_components-1;
  recs_entity entity_at_last_component = ca->comp_to_entity[last_component_index];


  //move last element to component being removed.
  //this keeps our list of components contiguous.
  //NOTE: Whether this has any performance benefits over allowing
  //our component pool buffer to be sparse is not tested.
  memcpy(
    ca->buffer + (ca->component_size * component_index),
    ca->buffer + (ca->component_size * last_component_index),
    ca->component_size
  );

  ca->comp_to_entity[component_index] = entity_at_last_component;
  ca->entity_to_comp[entity_at_last_component] = component_index;

  ca->comp_to_entity[last_component_index] = RECS_ENT_ID(e);
  ca->entity_to_comp[RECS_ENT_ID(e)] = NO_COMP_ID;

  ca->num_components--;


}


// END Internal functions



recs_entity recs_entity_add(struct recs *ecs) {
  RECS_ASSERT(ecs->ent_man.num_active_entities < ecs->ent_man.max_entities);

  recs_entity e = entity_manager_add(&ecs->ent_man);
  return e;
  
}

void recs_entity_remove(struct recs *ecs, recs_entity e) {
  if(RECS_ENT_ID(e) == RECS_NO_ENTITY_ID) return;

  //update version number
  if(RECS_ENT_VERSION(e) == ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)]) {
    ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)]++;
  }



  //delete components
  recs_entity_remove_all_components(ecs, e);

  //remove from active entity pool
  entity_manager_remove(&ecs->ent_man, e);

}

void recs_entity_queue_remove(struct recs *ecs, recs_entity e) {
  ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)]++;
}

void recs_entity_remove_queued(struct recs *ecs) {
  if(ecs->ent_man.num_active_entities == 0) return;

  //move in reversed order since the index does not need to be modified when removing entities
  for(uint32_t i = ecs->ent_man.num_active_entities; i > 0; i--) {
    recs_entity e = ecs->ent_man.entity_pool[i-1];
    if(recs_entity_active(ecs, e)) continue;

    //delete components
    recs_entity_remove_all_components(ecs, e);

    //remove from active entity pool
    entity_manager_remove_at_index(&ecs->ent_man, i-1);


  }
}


void recs_entity_add_component(struct recs *ecs, recs_entity e, recs_component comp_type, void *component) {

  struct component_pool *ca = ecs->recs_component_stores + comp_type;
  component_pool_add(ca, e, component);

  //set bit
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, e), comp_type, 1);
}

void recs_entity_add_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), recs_tag_id_to_comp_id(ecs, tag), 1);
}

void recs_entity_remove_component(struct recs *ecs, recs_entity e, recs_component comp_type) {
  struct component_pool *ca =  ecs->recs_component_stores + comp_type;
  component_pool_remove(ca, e);

  //clear bit
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), comp_type, 0);

}

void recs_entity_remove_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), recs_tag_id_to_comp_id(ecs, tag), 0);
}

void recs_entity_remove_all_components(struct recs *ecs, recs_entity e) {

  //remove components from component arrays
  for(recs_component t = 0; t < ecs->max_registered_components; t++) {
    if(bitmask_test(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), t)) {
      recs_entity_remove_component(ecs, e, t);

    }
  }

  //mark entity as having no components to clear tags
  bitmask_clear(bitmask_list_get(&ecs->comp_bitmask_list, e), 0, ecs->comp_bitmask_size);

}

int recs_entity_has_component(struct recs *ecs, recs_entity e, recs_component c) {
  return bitmask_test(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), c);
}

int recs_entity_has_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  return bitmask_test(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), recs_tag_id_to_comp_id(ecs, tag));
}

void* recs_entity_get_component(struct recs *ecs, recs_entity e, recs_component c) {
  return component_pool_get(ecs->recs_component_stores + c, e);
}

//components are densely packed, so you can retrieve them using an index
//if desired. Note that components will not stay at the same index when removing
//components, so make sure not to remove components when using this function
void* recs_component_get(struct recs *recs, recs_component c, uint32_t index) {
  struct component_pool *p = recs->recs_component_stores + c;
  return p->buffer + (index * p->component_size);
}



int recs_entity_matches_component_mask(struct recs *ecs, recs_entity e, uint8_t *mask, enum recs_ent_match_op match_op) {

  uint8_t *mask_for_entity = bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e));

  //check all bytes except last one
  for(uint32_t i = 0; i < ecs->comp_bitmask_size - 1; i++) {
    uint8_t res = mask_for_entity[i] & mask[i];
    switch(match_op) {
      case RECS_ENT_MATCH_ALL: {
        if(res != mask[i]) {
          return 0;
        }
        break;
      }
      case RECS_ENT_MATCH_ANY: {
        if(res != 0) {
          return 1;
        }
        break;
      }
    }
    
  }

  //only check the LSB on the last byte

  //get number of bits (starting from MSB) that we are setting to 0 so that we can check the equality of the LSB bits.
  
  //do not account for when mask_bits == 0, as this should not be possible
  //when mask_bits = 1, num_unused_bits = 7
  //when mask_bits = 2, num_unused_bits = 6
  //when mask_bits = 3, num_unused_bits = 5
  //when mask_bits = 4, num_unused_bits = 4
  //when mask_bits = 7, num_unused_bits = 1
  //when mask_bits = 8, num_unused_bits = 0
  //when mask_bits = 9, num_unused_bits = 7
  
  // if mask_bits % 8 != 0
    //num_unused_bits == 8 - (mask_bits % 8) == 8 - (mask_bits & 7)
  // else
  // num_unused_bits = 0
  
  const uint32_t mask_bits = ecs->max_registered_components + ecs->max_tags;
  const uint8_t num_unused_bits = (mask_bits & 7) == 0 ? 0 : 8 - (mask_bits & 7);




  //grab the last byte and AND with (0xFF >> num_unused_bits) to set all unused MSB bits to 0
  uint8_t last_byte1 = mask_for_entity[ecs->comp_bitmask_size-1] & ((uint8_t)0xFF >> num_unused_bits);
  uint8_t last_byte2 = mask[ecs->comp_bitmask_size-1] & ((uint8_t)0xFF >> num_unused_bits);

  switch(match_op) {
    case RECS_ENT_MATCH_ALL: {
      return (last_byte1 & last_byte2) == last_byte2;
    }
    case RECS_ENT_MATCH_ANY: {
      return (last_byte1 & last_byte2) != 0;
    }

  }
  return 0;
}



int recs_entity_has_components(struct recs *ecs, recs_entity e, uint8_t *mask) {
  return recs_entity_matches_component_mask(ecs, e, mask, RECS_ENT_MATCH_ALL);
}


int recs_entity_has_excluded_components(struct recs *ecs, recs_entity e, uint8_t *mask) {
  return !recs_entity_matches_component_mask(ecs, e, mask, RECS_ENT_MATCH_ANY);
}

uint8_t recs_entity_active(struct recs *ecs, recs_entity e) {
  return !RECS_ENTITY_NONE(e) && RECS_ENT_VERSION(e) == ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)];
}
