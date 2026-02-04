#ifndef TAGS_H
#define TAGS_H

#include "types.h"

//all tags appear AFTER the recs_components with data.
static inline uint32_t recs_tag_id_to_comp_id(struct recs *ecs, recs_tag tag) {
  uint32_t id = tag + ecs->max_registered_components;
  RECS_ASSERT(id < ecs->max_tags + ecs->max_registered_components);
  return id;
}


#endif// TAGS_H
