#include <stdio.h>

// can define own malloc(), free(), and assert() macros before including ecs.h
// #define RECS_MALLOC(size) <custom malloc() here>
// #define RECS_FREE(ptr) <custom free() here>
// #define RECS_ASSERT(boolean) <custom assert() here>

#define RECS_MAX_COMPONENTS 2
#define RECS_MAX_TAGS 2
#define RECS_MAX_ENTITIES 2
#define RECS_MAX_SYSTEMS 2
#define RECS_MAX_SYS_GROUPS 1

#include "recs.h"


//define components
struct message_component {
  char message[25];
};

struct number_component {
  uint64_t num;
};


// define components, tags, and system group enums.
// note that you can define these enums manually if desired, though you will need to 
// make sure enum values start at 0 and increment by 1.

RECS_INIT_COMP_IDS(component, COMPONENT_MESSAGE, COMPONENT_NUMBER, COMPONENT_COUNT);
RECS_INIT_TAG_IDS(tag, TAG_A, TAG_B, TAG_COUNT);
RECS_INIT_SYS_GRP_IDS(system_group, SYSTEM_GROUP_UPDATE);



//define our systems
void system_print_message(struct recs *ecs) {
  uint8_t exclude_mask[RECS_GET_BITMASK_SIZE(COMPONENT_COUNT, TAG_COUNT)];
  recs_bitmask_create(ecs, exclude_mask, 0, NULL, 0, NULL);

  //iterate through EVERY entity
  recs_ent_iter iter = recs_ent_iter_init_with_exclude(ecs, NULL, exclude_mask);

  while(recs_ent_iter_has_next(&iter)) {

    //grab our entity ID
    recs_entity e = recs_ent_iter_next(ecs, &iter);

    struct message_component *m = recs_entity_get_component(ecs, e, COMPONENT_MESSAGE);
    struct number_component *n = recs_entity_get_component(ecs, e, COMPONENT_NUMBER);

    //check if our entity has a specific component, if it does, do something with it.
    if(m != NULL) {
      printf("Message: %s\n", m->message);
    }

    if(n != NULL) {
      printf("Number: %llu\n", n->num);
    }

    const char *has_tag_a_str = recs_entity_has_tag(ecs, e, TAG_A) ? "true" : "false";
    const char *has_tag_b_str = recs_entity_has_tag(ecs, e, TAG_B) ? "true" : "false";

    printf("Has Tag A = %s\n", has_tag_a_str);
    printf("Has Tag B = %s\n", has_tag_b_str);
  }
}

void system_print_number_only(struct recs *ecs) {
  //allocate enough memory to store bitmask
  uint8_t mask[RECS_GET_BITMASK_SIZE(RECS_MAX_COMPONENTS, RECS_MAX_TAGS)];

  //initialize allocated bitmask with the tags and components we want to retrieve
  recs_bitmask_create(ecs, mask,
    RECS_BITMASK_CREATE_COMP_ARG(1, COMPONENT_NUMBER), 
    RECS_BITMASK_CREATE_TAG_ARG(2, TAG_A, TAG_B)
  );

  recs_ent_iter iter = recs_ent_iter_init(ecs, mask);

  //only iterate though entities with the COMPONENT_NUMBER component and the 
  //tags TAG_A and TAG_B. 
  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);
    struct number_component *n = recs_entity_get_component(ecs, RECS_ENT_ID(e), COMPONENT_NUMBER);
    printf("Entity %d with TAG_A and TAG_B has number %llu\n", RECS_ENT_ID(e), n->num);
  }
}

int main(void) {
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
    },
    {
      .func = system_print_number_only,
      .group = SYSTEM_GROUP_UPDATE
    }
  };

  struct recs_init_config config = {
    .max_entities = RECS_MAX_ENTITIES,
    .max_component_types = RECS_MAX_COMPONENTS,
    .max_tags = RECS_MAX_TAGS,
    .max_systems = RECS_MAX_SYSTEMS,
    .max_system_groups = RECS_MAX_SYS_GROUPS,
    .context = NULL,
    .components = comps,
    .systems = systems
  };
  //attempt to allocate and initialize our ECS
  struct recs *ecs = recs_init(config);

  //will fail if we fail to allocate enough memory for the ECS.
  if(ecs == NULL) {
    printf("Failed to initialize!\n");
    return 1;
  }


  //initialize an entity
  recs_entity e = recs_entity_add(ecs);

  //initialize and add this entity's components
  struct number_component n = {
    .num = 42
  };
  struct message_component m = {
    .message = "Hello, There"
  };

  recs_entity_add_component(ecs, e, COMPONENT_MESSAGE, &m);
  recs_entity_add_component(ecs, e, COMPONENT_NUMBER, &n);

  //assign 2 tags to this new entity
  recs_entity_add_tag(ecs, e, TAG_A);
  recs_entity_add_tag(ecs, e, TAG_B);
  

  //run all registered systems tagged with RECS_SYSTEM_TYPE_UPDATE
  //in the order they are registered in.
  recs_system_run(ecs, SYSTEM_GROUP_UPDATE);

  //queue 1st entity to be removed. Note that doing this also disables the entity,
  //which prevents it from being returned from iterators
  recs_entity_queue_remove(ecs, e);
  recs_system_run(ecs, SYSTEM_GROUP_UPDATE);


  //remove the queued entity from the active entity list
  recs_entity_remove_queued(ecs);


  //ecs needs to be freed once it is no longer needed.
  recs_free(ecs);
  return 0;
}

