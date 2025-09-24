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

RECS_INIT_COMP_IDS(component, COMPONENT_MESSAGE, COMPONENT_NUMBER);
RECS_INIT_TAG_IDS(tag, TAG_A, TAG_B);
RECS_INIT_SYS_GRP_IDS(system_group, SYSTEM_GROUP_UPDATE);



//define our systems
void system_print_message(struct recs *ecs) {

  //iterate through every entity
  for(uint32_t i = 0; i < recs_num_active_entities(ecs); i++) {

    //grab our entity ID
    recs_entity e = recs_entity_get(ecs, i);

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

  recs_ent_iter iter = recs_ent_iter_init(mask);

  //only iterate though entities with the COMPONENT_NUMBER component and the 
  //tags TAG_A and TAG_B. 
  while(recs_ent_iter_has_next(ecs, &iter)) {
    recs_ent_iter_next(ecs, &iter);
    struct number_component *n = recs_entity_get_component(ecs, iter.current_entity, COMPONENT_NUMBER);
    printf("Entity %d with TAG_A and TAG_B has number %llu\n", iter.current_entity, n->num);
  }
}

int main(void) {

  //attempt to allocate and initialize our ECS
  struct recs *ecs = recs_init(RECS_MAX_ENTITIES, RECS_MAX_COMPONENTS, RECS_MAX_TAGS, RECS_MAX_SYSTEMS, RECS_MAX_SYS_GROUPS, NULL);

  //will fail if we fail to allocate enough memory for the ECS.
  if(ecs == NULL) {
    printf("Failed to initialize!\n");
    return 1;
  }


  // register components
  uint8_t comp_register_failed = !recs_component_register(ecs, COMPONENT_MESSAGE, 10, sizeof(struct message_component));
  comp_register_failed |= !recs_component_register(ecs, COMPONENT_NUMBER, 10, sizeof(struct number_component));

  //note that component registration can fail if RECS_MALLOC() fails to 
  //allocate enough memory for it.
  if(comp_register_failed) {
    recs_free(ecs);
    printf("Failed to register component!\n");
    return 1;
  }

  //register system and assign it with the type "UPDATE"
  recs_system_register(ecs, system_print_message, SYSTEM_GROUP_UPDATE);
  recs_system_register(ecs, system_print_number_only, SYSTEM_GROUP_UPDATE);


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

  //remove the 1st entity
  recs_entity_remove(ecs, e);


  //ecs needs to be freed once it is no longer needed.
  recs_free(ecs);
  return 0;
}

