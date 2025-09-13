# RECS (Ryan's Entity Component System)

> Note that this library is still a work-in-progress, so expect breaking changes to be introduced in future updates until I consider this library stable.

This is a basic implementation of a generic Entity-Component-System. This uses sparse sets and component pools for storing component data and mapping entity IDs to their components.

## Quick Explanation of What A Entity Component System (ECS) Is:

An Entity-Component-System (ECS) is a software architecture pattern that is primarily used in video game development to represent game world object. It is comprised of three data structures:
1. Entity
  - An entity is composed of multiple components
  - It is usually represented as a single numeric ID in memory.
2. Component
  - A component can hold any kind of data
  - It does not have any behaviors associated with them
  - Each entity can only be given 1 instance of each component type
    - If you want multiple instances of a component for a single entity, you have a few options:
      - You can create an aggregate component that stores multiple instances of the same component.
      - You can convert the original component into an aggregate component that stores multiple instances of a generic data structure with some helper functions associated with it. This avoids the need to write a system for both the single component and the aggregate component, since you now only have the aggregate component.
      - You can convert the original component into a generic data structure, then use them in more specialized components.
3. System
  - A system is a function that grabs all entities with a specific set of components and performs actions using these components, such as performing in-game updates, having entities interact with each other, and adding and removing entities to the game world.


By organizing your game objects and their behaviors this way, you can easily add and remove components at any point to any entity. Rather than writing specific code for each entity type, you write a system that interacts with all entities that share the same set of component types at once, regardless of the entity type. This modularity makes it very easy to build new entities out of already existing components and allows changes to components/systems to propagate to all entities that use that component.
 



## Features:
  - Add and remove entities
  - Attach components and tags to entities
  - Register systems and assign them one of 2 tags:
    - ECS_SYSTEM_TYPE_UPDATE
      - Used to run game logic and physics updates
    - ECS_SYSTEM_TYPE_RENDER
      - Used to render entities to the screen

  - Support for entity tags, which are essentially components with no attached data
  - Users can add custom malloc(), free(), and assert() implementations into this library
    by overwriting the MALLOC, FREE, and ASSERT macros.

  - Size Limitations
    - Maximum of 2^32 - 1 entities.
    - Maximum of 2^32 component types and tags.
    - Maximum of 2^32 - 1 component instances per component type.
    - Maximum of 2^32 systems.


## Sample Usage


```
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

```

## External Resources About ECS and Other ECS Projects
- https://en.wikipedia.org/wiki/Entity_component_system
- https://github.com/SanderMertens/ecs-faq
- https://github.com/SanderMertens/flecs