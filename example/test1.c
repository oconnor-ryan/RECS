#include <stdio.h>

// can define own MALLOC, FREE, and ASSERT macros before including ecs.h
// #define MALLOC(size) <custom malloc() here>
// #define FREE(ptr) <custom free() here>
// #define ASSERT(boolean) <custom assert() here>

#include "ecs.h"


//a helper macro to free our ECS and exit the program if we fail to allocate enough memory

#define FREE_AND_FAIL(ecs, message) do {printf("%s", message); ecs_free(&ecs); return 1;} while(0) 


//define components
struct message_component {
  char message[25];
};

struct number_component {
  uint64_t num;
};


// define component IDs
enum component {
  COMPONENT_MESSAGE, //associated with 'struct message_component'
  COMPONENT_NUMBER   //associated with 'struct number_component'
};


//define our systems
void system_print_message(struct ecs *ecs, void *context) {

  //iterate through every entity
  for(uint32_t i = 0; i < ecs->ent_man.num_active_entities; i++) {

    //grab our entity ID
    entity e = ecs_entity_get(ecs, i);

    //check if our entity has a specific component, if it does, grab that component
    //and do something with it.
    if(ecs_entity_has_component(ecs, e, COMPONENT_MESSAGE)) {
      struct message_component *m = (struct message_component *)ecs_entity_get_component(ecs, e, COMPONENT_MESSAGE);
      printf("Message: %s\n", m->message);
    }

    if(ecs_entity_has_component(ecs, e, COMPONENT_NUMBER)) {
      struct number_component *m = (struct number_component *)ecs_entity_get_component(ecs, e, COMPONENT_NUMBER);
      printf("Number: %llu\n", m->num);
    }
  }
}

int main(void) {

  //attempt to allocate and initialize our ECS
  struct ecs ecs;
  int init_success = ecs_init(&ecs, 10, 10, 10, 10);

  //will fail if we fail to allocate enough memory for the ECS.
  if(!init_success) {
    printf("Failed to initialize!\n");
    return 1;
  }


  // register components
  // note that registration can fail if we cannot allocate enough memory to store our
  // component pool
  if(!ecs_component_register(&ecs, COMPONENT_MESSAGE, 10, sizeof(struct message_component))) {
    FREE_AND_FAIL(ecs, "Failed to register component\n");
  }
  if(!ecs_component_register(&ecs, COMPONENT_NUMBER, 10, sizeof(struct number_component))) {
    FREE_AND_FAIL(ecs, "Failed to register component\n");
  }

  //register system and assign it with the type "UPDATE"
  ecs_system_register(&ecs, system_print_message, NULL, ECS_SYSTEM_TYPE_UPDATE);


  //initialize an entity and attach its components.
  entity e = ecs_entity_add(&ecs);
  struct message_component m = {
    .message = "Hi there"
  };
  ecs_entity_add_component(&ecs, e, COMPONENT_MESSAGE, &m);

  struct number_component n = {
    .num = 5
  };
  ecs_entity_add_component(&ecs, e, COMPONENT_NUMBER, &n);


  //run all systems tagged with system type UPDATE
  ecs_system_run_all_with_type(&ecs, ECS_SYSTEM_TYPE_UPDATE);

  //ecs needs to be freed once it is no longer needed.
  ecs_free(&ecs);
  return 0;
}
