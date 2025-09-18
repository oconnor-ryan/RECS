# RECS (Ryan's Entity Component System)

> Note that this library is still a work-in-progress, so expect breaking changes to be introduced in future updates until I consider this library stable.

This is a basic implementation of a generic Entity-Component-System. This uses sparse sets and component pools for storing component data and mapping entity IDs to their components. 

## Building the Library

You will need the following dependencies installed in order to build
this project from source:
- CMake >= 3.15
- A C compiler that supports C11 standard.

Before building with CMake, you must set up your build folder using:
`cmake -S . -B build`

To build only the RECS library:
`cmake --build build --target recs`

To build the example code for using the RECS library:
`cmake --build build --target example1`
`cmake --build build --target example2`


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
    - `RECS_SYSTEM_TYPE_UPDATE`
      - Used to run game logic and physics updates
    - `RECS_SYSTEM_TYPE_RENDER`
      - Used to render entities to the screen

  - Support for entity tags, which are essentially components with no attached data
  - Users can add custom malloc(), free(), and assert() implementations into this library
    by overwriting the RECS_MALLOC, RECS_FREE, and RECS_ASSERT macros.

  - Size Limitations
    - Maximum of 2^32 - 1 entities.
    - Maximum of 2^32 component types and tags.
    - Maximum of 2^32 - 1 component instances per component type.
    - Maximum of 2^32 systems.


## Sample Usage


```c
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

int main(void) {

  //attempt to allocate and initialize our ECS
  struct recs *ecs = recs_init(2, 2, 2, 2);

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
  recs_system_register(ecs, system_print_message, NULL, RECS_SYSTEM_TYPE_UPDATE);


  //initialize an entity
  entity e = recs_entity_add(ecs);

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
  recs_system_run_all_with_type(ecs, RECS_SYSTEM_TYPE_UPDATE);

  //remove the 1st entity
  recs_entity_remove(ecs, e);


  //ecs needs to be freed once it is no longer needed.
  recs_free(ecs);
  return 0;
}

```

## Potential Upcoming Features
- Allow user to define system types (rather than only being able to label them `RECS_SYSTEM_TYPE_UPDATE` and `RECS_SYSTEM_TYPE_RENDER`).
- Allow more efficient way to query entities within systems based on their components and tags.
  - Perhaps users could tell the ECS what types of queries they want to make on initialization of ECS, that way each entity gets stored within a specific array containing entities with the same set of elements.

## External Resources About ECS and Other ECS Projects
- https://en.wikipedia.org/wiki/Entity_component_system
- https://github.com/SanderMertens/ecs-faq
- https://github.com/SanderMertens/flecs