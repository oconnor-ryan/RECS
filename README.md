# RECS (Ryan's Entity Component System)

> Note that this library is still a work-in-progress, so expect breaking changes to be introduced in future updates until I consider this library stable.

This is a basic implementation of a generic Entity-Component-System. This uses sparse sets and component pools for storing component data and mapping entity IDs to their components. 

## Building This Library

To build the RECS library, you will need the following dependencies:
- CMake >= 3.15
- A C compiler that supports C99 standard

Before building with CMake, you must set up your build folder using:
`cmake -S . -B build`


To build the library, you will run:
`cmake --build build --target recs`

Once you generate the library file, you will also need to 
grab the `recs.h` file within the `src` directory and include it into
your project. This header allows you to interface with the library you
just generated.


## Building The Example Code (Optional)

You will need the following dependencies installed in order to build
the examples that I included:
- CMake >= 3.15
- A C compiler that supports C11 standard (some examples utilize C11 features for convenience)

Before building with CMake, you must set up your build folder using:
`cmake -S . -B build`

To build the example code for using the RECS library:
`cmake --build build --target example1`
`cmake --build build --target example2`


## Running The Tests

You will need the following dependencies installed in order to build
the tests that I included:
- CMake >= 3.15
- A C compiler that supports C11 standard (some examples utilize C11 features for convenience)

Before building with CMake, you must set up your build folder using:
`cmake -S . -B build`

Here are the list of all testing targets:

To build the test code, you will need to run:
`cmake --build build --target build_tests`

To run all tests, you can use CTest to run this command:
`ctest --test-dir build --output-on-failure`


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


## Sample Program


```c
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

  recs_ent_iter iter = recs_ent_iter_init(ecs, mask);

  //only iterate though entities with the COMPONENT_NUMBER component and the 
  //tags TAG_A and TAG_B. 
  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);
    struct number_component *n = recs_entity_get_component(ecs, e, COMPONENT_NUMBER);
    printf("Entity %d with TAG_A and TAG_B has number %llu\n", e, n->num);
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
```

## Current Feature Ideas
While playing with this ECS in my own project, I found a few issues that make this ECS less user-friendly.

- Make adding and removing entities less error-prone. 
  - Currently, if you try to add and remove entities while iterating through entities in a system, then strange behavior can occur (skipping entities during iterations, entities being processed before they even spawn in).
  - A solution is to use a built-in "DEFER_SPAWN" and "DEFER_FREE" tag. Rather than being able to directly add
    and remove entities, you will be required to queue these actions instead until a point where there are no systems running.
    - While this forces all operations to be queued to after a system executes, it makes removing and adding entities
      consistant regardless of when you perform these operations.

- Figure out how to update references to entities within components
  - Currently, if a entity is deleted, any components referencing this entity will not know that the entity there are refering to no longer exists, and the ID may point to a different entity. 
  - A solution is to extend the Entity number to 64 bits, and split it into a "version" number and an actual ID. When the entity is deleted, its version is increased for that specific entity ID. The component storing the 64-bit entity ID retains an older version of the ID, so it knows to not track this entity anymore.
    - Even if the version number overflows, because we are checking that the version is EQUAL to the current version, overflow should not matter except for the rare case where we somehow remove and spawn an entity with the same ID exactly 2^32 times during the lifespan of a component, which will likely never happen.
    - This does require storing (sizeof(uint32_t) * max_entities) version numbers, but this is a small price to pay for automatically updating the state of each entity ID for components.

## Potential Upcoming Features
- Allow more efficient way to query entities within systems based on their components and tags.
  - Perhaps users could tell the ECS what types of queries they want to make on initialization of ECS, that way each entity gets stored within a specific array containing entities with the same set of elements.



## External Resources About ECS and Other ECS Projects
- https://en.wikipedia.org/wiki/Entity_component_system
- https://github.com/SanderMertens/ecs-faq
- https://github.com/SanderMertens/flecs