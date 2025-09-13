#include <stdio.h>

#include "ecs.h"


#define FREE_AND_FAIL(ecs, message) do {printf("%s", message); ecs_free(&ecs); return 1;} while(0) 
struct message_component {
  char message[25];
};

struct number_component {
  uint64_t num;
};


enum component {
  COMPONENT_MESSAGE,
  COMPONENT_NUMBER
};

void system_print_message(struct ecs *ecs, void *context) {

  for(uint32_t i = 0; i < ecs->ent_man.num_active_entities; i++) {
    entity e = ecs_entity_get(ecs, i);
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

  struct ecs ecs;
  int init_success = ecs_init(&ecs, 10, 10, 10, 10);

  if(!init_success) {
    printf("Failed to initialize!\n");
    return 1;
  }
  // register components
  if(!ecs_component_register(&ecs, COMPONENT_MESSAGE, 10, sizeof(struct message_component))) {
    FREE_AND_FAIL(ecs, "Failed to register component\n");
  }
  if(!ecs_component_register(&ecs, COMPONENT_NUMBER, 10, sizeof(struct number_component))) {
    FREE_AND_FAIL(ecs, "Failed to register component\n");
  }

  //register system
  ecs_system_register(&ecs, system_print_message, NULL, ECS_SYSTEM_TYPE_UPDATE);


  entity e = ecs_entity_add(&ecs);
  struct message_component m = {
    .message = "Hi there"
  };
  ecs_entity_add_component(&ecs, e, COMPONENT_MESSAGE, &m);

  struct number_component n = {
    .num = 5
  };
  ecs_entity_add_component(&ecs, e, COMPONENT_NUMBER, &n);

  ecs_system_run_all_with_type(&ecs, ECS_SYSTEM_TYPE_UPDATE);

  ecs_free(&ecs);
  return 0;
}
