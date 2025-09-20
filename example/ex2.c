#include <stdio.h>


#define RECS_MAX_COMPONENTS 2
#define RECS_MAX_TAGS 2
#define RECS_MAX_ENTITIES 2
#define RECS_MAX_SYSTEMS 2
#define RECS_MAX_SYS_GROUPS 2
#define RECS_IMPLEMENTATION

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
  You have the option to not use any of the macros defined in the RECS library, such as:
  RECS_INIT_COMP_IDS
  RECS_INIT_TAG_IDS
  RECS_INIT_SYS_GRP_IDS
  RECS_COMP_TO_ID_MAPPER,
  RECS_MAP_COMP_TO_ID,
  RECS_MAP_COMP_PTR_TO_ID,
  RECS_ENTITY_ADD_COMP

  However, you will need to be careful to ensure that:
  - All enums for tags, components, and system_groups have values that start at 0 and increase by 1 (sinc
  these are indices to arrays)
  - Remember which component enum matches with what data structure or type you used to define the component with.



*/

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
  //grab context pointer from ECS and convert to appropriate type
  int *num_from_context_ptr = (int*)recs_system_get_context(ecs);

  printf("========== System Print Message %d ==========\n", *num_from_context_ptr);

  //iterate through every entity
  for(uint32_t i = 0; i < recs_num_active_entities(ecs); i++) {

    //grab our entity ID
    recs_entity e = recs_entity_get(ecs, i);

    printf("Entity Id: %u\n", e);

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

    printf("End of Entity %u\n\n", e);

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

  //attempt to allocate and initialize our ECS, along with setting the context pointer.
  struct recs *ecs = recs_init(&num_updates);

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
  recs_system_register(ecs, system_print_message, SYSTEM_GROUP_UPDATE);


  //initialize an entity and attach its components.
  recs_entity a = entity_factory_a(ecs, 1, "Hi");
  recs_entity b = entity_factory_a(ecs, 2, "There");
  run_update(ecs);
  

  //remove the 1st entity
  recs_entity_remove(ecs, a);
  run_update(ecs);


  //remove the 2nd entity
  recs_entity_remove(ecs, b);
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
