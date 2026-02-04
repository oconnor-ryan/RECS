#include <stdio.h>


#define RECS_MAX_COMPONENTS 2
#define RECS_MAX_TAGS 2
#define RECS_MAX_ENTITIES 2
#define RECS_MAX_SYSTEMS 1
#define RECS_MAX_SYS_GROUPS 1

#include "recs.h"


//a helper macro to free our ECS and exit the program if we fail to allocate enough memory

#define FREE_AND_FAIL(ecs, message) do {printf("%s", message); recs_free(ecs); return 1;} while(0) 


//define components
struct message_component {
  char message[25];
};

struct number_component {
  uint64_t num;
};


/*
  Make sure that the enums used for components fall in 
  the range 0 to YOUR_MAX_COMPONENTS-1, inclusive.
*/
enum component {
  COMPONENT_MESSAGE, //associated with 'struct message_component'
  COMPONENT_NUMBER,   //associated with 'struct number_component'


  COMPONENT_COUNT
};

/*
  Make sure that the enums used for tags fall in 
  the range 0 to YOUR_MAX_TAGS-1, inclusive.
*/
enum tag {
  TAG_A,
  TAG_B,


  TAG_COUNT
};


/*
  Make sure that the enums used for system groups fall in 
  the range 0 to YOUR_MAX_SYSTEM_GROUPS-1, inclusive.
*/
enum system_group {
  SYSTEM_GROUP_UPDATE
};

//define our systems
void system_print_message(struct recs *ecs) {
  //grab context pointer from ECS and convert to appropriate type
  int *num_from_context_ptr = (int*)recs_system_get_context(ecs);

  printf("========== System Print Message %d ==========\n", *num_from_context_ptr);

  uint8_t exclude_mask[RECS_GET_BITMASK_SIZE(COMPONENT_COUNT, TAG_COUNT)];
  recs_bitmask_create(ecs, exclude_mask, 0, NULL, 0, NULL);

  //iterate through EVERY entity
  recs_ent_iter iter = recs_ent_iter_init_with_exclude(ecs, NULL, exclude_mask);

  while(recs_ent_iter_has_next(&iter)) {

    //grab our entity ID
    recs_entity e = recs_ent_iter_next(ecs, &iter);

    printf("Entity Id: %u\n", RECS_ENT_ID(e));

    //check if our entity has a specific component, if it does, grab that component
    //and do something with it.
    if(recs_entity_has_component(ecs, e, COMPONENT_MESSAGE)) {
      struct message_component *m = (struct message_component *)recs_entity_get_component(ecs, e, COMPONENT_MESSAGE);
      printf("Message: %s\n", m->message);
    }

    if(recs_entity_has_component(ecs, e, COMPONENT_NUMBER)) {
      struct number_component *m = (struct number_component *)recs_entity_get_component(ecs, e, COMPONENT_NUMBER);
      printf("Number: %llu\n", m->num);
    }

    const char *has_tag_a_str = recs_entity_has_tag(ecs, e, TAG_A) ? "true" : "false";
    const char *has_tag_b_str = recs_entity_has_tag(ecs, e, TAG_B) ? "true" : "false";

    printf("Has Tag A = %s\n", has_tag_a_str);
    printf("Has Tag B = %s\n", has_tag_b_str);

    printf("End of Entity %u\n\n", RECS_ENT_ID(e));

  }

  printf("============================================\n\n");

  //modify value in context pointer
  (*num_from_context_ptr)++;



}

recs_entity entity_factory_a(struct recs *ecs, uint64_t num, char message[25]) {
  struct number_component n = {
    .num = num
  };
  struct message_component m;

  for(uint32_t i = 0; i < 25; i++) {
    m.message[i] = message[i];
  }

  recs_entity e = recs_entity_add(ecs);

  recs_entity_add_component(ecs, e, COMPONENT_MESSAGE, &m);
  recs_entity_add_component(ecs, e, COMPONENT_NUMBER, &n);

  return e;
}

void run_update(struct recs *ecs) {

  //run all systems tagged with system type UPDATE
  recs_system_run(ecs, SYSTEM_GROUP_UPDATE);
}

int main(void) {

  int num_updates = 1;

  struct recs_init_config_component comps[RECS_MAX_COMPONENTS] = {
    {
      .type = COMPONENT_MESSAGE,
      .max_components = RECS_MAX_ENTITIES,
      .comp_size = sizeof(struct message_component)
    },
    {
      .type = COMPONENT_NUMBER,
      .max_components = RECS_MAX_ENTITIES,
      .comp_size = sizeof(struct number_component)
    }
  };

  struct recs_init_config_system systems[RECS_MAX_SYSTEMS] = {
    {
      .func = system_print_message,
      .group = SYSTEM_GROUP_UPDATE
    }
  };

  struct recs_init_config config = {
    .max_entities = RECS_MAX_ENTITIES,
    .max_component_types = RECS_MAX_COMPONENTS,
    .max_tags = RECS_MAX_TAGS,
    .max_systems = RECS_MAX_SYSTEMS,
    .max_system_groups = RECS_MAX_SYS_GROUPS,
    .context = &num_updates,
    .components = comps,
    .systems = systems
  };

  //attempt to allocate and initialize our ECS, along with setting the context pointer.
  struct recs *ecs = recs_init(config);

  //will fail if we fail to allocate enough memory for the ECS.
  if(ecs == NULL) {
    printf("Failed to initialize!\n");
    return 1;
  }



  //initialize an entity and attach its components.
  recs_entity a = entity_factory_a(ecs, 1, "Hi");
  recs_entity b = entity_factory_a(ecs, 2, "There");
  run_update(ecs);
  

  //remove the 1st entity
  recs_entity_queue_remove(ecs, a);

  //note that entities queued for removal are still "active", though they do not appear
  //within any entity iterators. You do need to remove them properly with the following function
  recs_entity_remove_queued(ecs);


  run_update(ecs);


  //remove the 2nd entity
  recs_entity_queue_remove(ecs, b);
  recs_entity_remove_queued(ecs);
  run_update(ecs);


  //add a new entity
  recs_entity c = entity_factory_a(ecs, 3, "Again");

  //assign 2 tags to this new entity
  recs_entity_add_tag(ecs, c, TAG_A);
  recs_entity_add_tag(ecs, c, TAG_B);

  run_update(ecs);


  //remove the message component from entity c
  recs_entity_remove_component(ecs, c, COMPONENT_MESSAGE);
  run_update(ecs);

  //remove TAG_A from entity c
  recs_entity_remove_tag(ecs, c, TAG_A);

  run_update(ecs);


  //ecs needs to be freed once it is no longer needed.
  recs_free(ecs);
  return 0;
}
