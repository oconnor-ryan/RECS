#include <stdio.h>

// can define own malloc(), free(), and assert() macros before including ecs.h
// #define RECS_MALLOC(size) <custom malloc() here>
// #define RECS_FREE(ptr) <custom free() here>
// #define RECS_ASSERT(boolean) <custom assert() here>

#include "recs.h"


//define components
struct message_component {
  char message[25];
};

struct number_component {
  uint64_t num;
};


// define components and tags as separate enums.
// Make sure that the values of component and tag enums start at 0, 
// and increment by 1 (this is what C does with enums by default)

enum component {
  COMPONENT_MESSAGE, //associated with 'struct message_component'
  COMPONENT_NUMBER   //associated with 'struct number_component'
};

enum tag {
  TAG_A,
  TAG_B
};

enum system_group {
  SYSTEM_GROUP_UPDATE
};

//define our systems
void system_print_message(struct recs *ecs) {

  //iterate through every entity
  for(uint32_t i = 0; i < recs_num_active_entities(ecs); i++) {

    //grab our entity ID
    recs_entity e = recs_entity_get(ecs, i);

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
  }
}

int main(void) {

  //attempt to allocate and initialize our ECS
  struct recs *ecs = recs_init(2, 2, 2, 2, 2, NULL);

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

