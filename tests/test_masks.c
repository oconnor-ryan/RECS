#include <stdio.h>

// can define own malloc(), free(), and assert() macros before including ecs.h
// #define RECS_MALLOC(size) <custom malloc() here>
// #define RECS_FREE(ptr) <custom free() here>
// #define RECS_ASSERT(boolean) <custom assert() here>



#define RECS_MAX_COMPONENTS 2
#define RECS_MAX_TAGS 2
#define RECS_MAX_ENTITIES 10
#define RECS_MAX_SYSTEMS 1
#define RECS_MAX_SYS_GROUPS 1


#include "recs.h"


struct entity_list {
  uint32_t num_entities;
  recs_entity list[3];
};


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


//optional. This just maps a pointer variable's type to the component enum
//we set in the previous few declarations.
#define MAP_COMP_PTR_TO_ID(var_ptr) _Generic((*var_ptr), \
  struct message_component: COMPONENT_MESSAGE, \
  struct number_component: COMPONENT_NUMBER)

  

void system_print_number_only(struct recs *ecs) {
  struct entity_list *test_list = recs_system_get_context(ecs);

  uint8_t mask[RECS_GET_BITMASK_SIZE(RECS_MAX_COMPONENTS, RECS_MAX_TAGS)];
  recs_bitmask_create(ecs, mask,
    RECS_BITMASK_CREATE_COMP_ARG(1, COMPONENT_NUMBER), 
    RECS_BITMASK_CREATE_TAG_ARG(2, TAG_A, TAG_B)
  );

  uint8_t exclude_mask[RECS_GET_BITMASK_SIZE(RECS_MAX_COMPONENTS, RECS_MAX_TAGS)];
  recs_bitmask_create(ecs, exclude_mask, RECS_BITMASK_CREATE_COMP_ARG(1, COMPONENT_MESSAGE), 0, NULL);


  recs_ent_iter iter = recs_ent_iter_init_with_exclude(ecs, mask, exclude_mask);

  //only iterate though entities with the COMPONENT_NUMBER component and the 
  //tags TAG_A and TAG_B. 
  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);
    struct number_component *n = recs_entity_get_component(ecs, e, MAP_COMP_PTR_TO_ID(n));
    printf("Entity %llu with TAG_A and TAG_B has number %llu\n", e, n->num);

    //record entity in testing list
    test_list->list[test_list->num_entities] = e;
    test_list->num_entities++;

  }
}

recs_entity make_entity_a(recs ecs) {
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

  return e;
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
      .func = system_print_number_only,
      .group = SYSTEM_GROUP_UPDATE
    },
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

  recs ecs = recs_init(config);

  //attempt to allocate and initialize our ECS

  //will fail if we fail to allocate enough memory for the ECS.
  if(ecs == NULL) {
    printf("Failed to initialize!\n");
    return 1;
  }



  recs_entity a = make_entity_a(ecs);
  recs_entity b = make_entity_a(ecs);
  recs_entity c = make_entity_a(ecs);

  //all three entities should be discovered by the iterator
  struct entity_list correct_entity_list = {
    .num_entities = 3,
    .list = {a,b,c}
  };


  //store list of entities that the iterator went through
  struct entity_list test_entity_list = {
    .num_entities = 0,
    .list = {RECS_NO_ENTITY_ID, RECS_NO_ENTITY_ID, RECS_NO_ENTITY_ID}
  };

  recs_system_set_context(ecs, &test_entity_list);

  //remove each component so that the iterator can use the exclude_mask to ensure that
  //the entities it iterates though does NOT have the COMPONENT_MESSAGE
  recs_entity_remove_component(ecs, b, COMPONENT_MESSAGE);
  recs_entity_remove_component(ecs, a, COMPONENT_MESSAGE);
  recs_entity_remove_component(ecs, c, COMPONENT_MESSAGE);



  //run all registered systems tagged with RECS_SYSTEM_TYPE_UPDATE
  //in the order they are registered in.
  recs_system_run(ecs, SYSTEM_GROUP_UPDATE);


  //ecs needs to be freed once it is no longer needed.
  recs_free(ecs);


  //check if test succeeded
  if(test_entity_list.num_entities != correct_entity_list.num_entities) {
    printf("Test Failed, not all valid entities were iterated though!\n");
    return 1;
  }
  for(uint32_t i = 0; i < correct_entity_list.num_entities; i++) {
    if(test_entity_list.list[i] != correct_entity_list.list[i]) {
      printf("Test Failed, Entity that was iterated though does not match the correct entity ID!\n");
      return 1;
    }
  }


  return 0;
}

