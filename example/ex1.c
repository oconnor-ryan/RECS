#include <stdio.h>

// can define own malloc(), free(), and assert() macros before including recs.h
// #define RECS_MALLOC(size) malloc(size)
// #define RECS_FREE(ptr) free(ptr)
// #define RECS_ASSERT(boolean) assert(boolean)

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


// define component IDs. Make sure the components and tags
// are valued starting at 0, 1, 2, ... (this is what C does by default).

enum component {
  COMPONENT_MESSAGE, //associated with 'struct message_component'
  COMPONENT_NUMBER   //associated with 'struct number_component'
};

enum tag {
  TAG_A,
  TAG_B
};

//define our systems
void system_print_message(struct recs *ecs, void *context) {

  //iterate through every entity
  for(uint32_t i = 0; i < recs_num_active_entities(ecs); i++) {

    //grab our entity ID
    entity e = recs_entity_get(ecs, i);

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

entity entity_factory_a(struct recs *ecs, uint64_t num, char message[25]) {
  struct number_component n = {
    .num = num
  };
  struct message_component m;

  for(uint32_t i = 0; i < 25; i++) {
    m.message[i] = message[i];
  }

  entity e = recs_entity_add(ecs);

  recs_entity_add_component(ecs, e, COMPONENT_MESSAGE, &m);
  recs_entity_add_component(ecs, e, COMPONENT_NUMBER, &n);

  return e;
}

void run_update(struct recs *ecs) {
  static uint32_t num_updates = 1;

  printf("Run System %d: \n===================\n", num_updates);
  //run all systems tagged with system type UPDATE
  recs_system_run_all_with_type(ecs, RECS_SYSTEM_TYPE_UPDATE);
  printf("===================\nEnd System %d \n\n", num_updates);

  num_updates++;
}

int main(void) {

  //attempt to allocate and initialize our ECS
  struct recs *ecs = recs_init(2, 2, 2, 2);

  //will fail if we fail to allocate enough memory for the ECS.
  if(ecs == NULL) {
    printf("Failed to initialize!\n");
    return 1;
  }


  // register components
  // note that registration can fail if we cannot allocate enough memory to store our
  // component pool
  if(!recs_component_register(ecs, COMPONENT_MESSAGE, 10, sizeof(struct message_component))) {
    FREE_AND_FAIL(ecs, "Failed to register component\n");
  }
  if(!recs_component_register(ecs, COMPONENT_NUMBER, 10, sizeof(struct number_component))) {
    FREE_AND_FAIL(ecs, "Failed to register component\n");
  }

  //register system and assign it with the type "UPDATE"
  recs_system_register(ecs, system_print_message, NULL, RECS_SYSTEM_TYPE_UPDATE);


  //initialize an entity and attach its components.
  entity a = entity_factory_a(ecs, 1, "Hi");
  entity b = entity_factory_a(ecs, 2, "There");
  run_update(ecs);
  

  //remove the 1st entity
  recs_entity_remove(ecs, a);
  run_update(ecs);


  //remove the 2nd entity
  recs_entity_remove(ecs, b);
  run_update(ecs);


  //add a new entity
  entity c = entity_factory_a(ecs, 3, "Again");

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
