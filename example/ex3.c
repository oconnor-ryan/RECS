#include <stdio.h>

// can define own malloc(), free(), and assert() macros before including recs.h
// #define RECS_MALLOC(size) malloc(size)
// #define RECS_FREE(ptr) free(ptr)
// #define RECS_ASSERT(boolean) assert(boolean)

#define RECS_MAX_COMPONENTS 2
#define RECS_MAX_TAGS 2
#define RECS_MAX_ENTITIES 2
#define RECS_MAX_SYSTEMS 3
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



RECS_INIT_COMP_IDS(component, COMPONENT_MESSAGE, COMPONENT_NUMBER);
RECS_INIT_TAG_IDS(tag, TAG_A, TAG_B);
RECS_INIT_SYS_GRP_IDS(system_group, SYSTEM_GROUP_A, SYSTEM_GROUP_B);



//user must define mapper to convert types to ids in order to use RECS_MAP_COMP_TO_ID macro. Note that the 
//compiler will warn you for not defining this automatically when using RECS_MAP_COMP_TO_ID().
#define RECS_COMP_TO_ID_MAPPER \
  struct message_component: COMPONENT_MESSAGE, \
  struct number_component: COMPONENT_NUMBER 



//define our systems
void system_print_message(struct recs *ecs) {

  printf("========== System Print Message ==========\n");

  //iterate through every entity
  for(uint32_t i = 0; i < recs_num_active_entities(ecs); i++) {

    //grab our entity ID
    recs_entity e = recs_entity_get(ecs, i);

    printf("Entity Id: %u\n", e);

    //check if our entity has a specific component, if it does, grab that component
    //and do something with it.
    if(recs_entity_has_component(ecs, e, COMPONENT_MESSAGE)) {
      struct message_component *m = recs_entity_get_component(ecs, e, RECS_MAP_COMP_PTR_TO_ID(m));

      printf("Message: %s\n", m->message);
    }
    printf("End of Entity %u\n\n", e);

  }

  printf("============================================\n\n");
}

void system_print_number(struct recs *ecs) {

  printf("========== System Print Number ==========\n");

  //iterate through every entity
  for(uint32_t i = 0; i < recs_num_active_entities(ecs); i++) {

    //grab our entity ID
    recs_entity e = recs_entity_get(ecs, i);

    printf("Entity Id: %u\n", e);

    if(recs_entity_has_component(ecs, e, COMPONENT_NUMBER)) {
      struct number_component *m = recs_entity_get_component(ecs, e, RECS_MAP_COMP_PTR_TO_ID(m));
      printf("Number: %llu\n", m->num);
    }
    printf("End of Entity %u\n\n", e);

  }

  printf("============================================\n\n");
}

void system_print_tags(struct recs *ecs) {
  printf("========== System Print Tags ==========\n");

  //iterate through every entity
  for(uint32_t i = 0; i < recs_num_active_entities(ecs); i++) {

    //grab our entity ID
    recs_entity e = recs_entity_get(ecs, i);

    printf("Entity Id: %u\n", e);

    const char *has_tag_a_str = recs_entity_has_tag(ecs, e, TAG_A) ? "true" : "false";
    const char *has_tag_b_str = recs_entity_has_tag(ecs, e, TAG_B) ? "true" : "false";

    printf("Has Tag A = %s\n", has_tag_a_str);
    printf("Has Tag B = %s\n", has_tag_b_str);

    printf("End of Entity %u\n\n", e);
  }

  printf("============================================\n\n");
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
  printf("************** System Group A **************\n\n");
  recs_system_run(ecs, SYSTEM_GROUP_A);
  printf("\n************** End Of Group A **************\n\n");


  printf("************** System Group B **************\n\n");
  recs_system_run(ecs, SYSTEM_GROUP_B);
  printf("\n************** End Of Group B **************\n\n");


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
  recs_system_register(ecs, system_print_message, SYSTEM_GROUP_A);
  recs_system_register(ecs, system_print_number, SYSTEM_GROUP_B);
  recs_system_register(ecs, system_print_tags, SYSTEM_GROUP_A);




  //initialize an entity and attach its components.
  recs_entity a = entity_factory_a(ecs, 1, "Hi");
  recs_entity b = entity_factory_a(ecs, 2, "There");
  recs_entity_add_tag(ecs, a, TAG_A);
  recs_entity_add_tag(ecs, b, TAG_B);

  run_update(ecs);
  

  //remove the 1st entity
  recs_entity_remove(ecs, a);
  run_update(ecs);


  //ecs needs to be freed once it is no longer needed.
  recs_free(ecs);
  return 0;
}
