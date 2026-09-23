#if defined(__GNUC__)
  //https://stackoverflow.com/a/69181103
  //Basically, if the --std=cXX compiler option is defined in GCC, it tries to hide any platform-specific or compiler-specific
  //headers and features. If you want to use a platform-dependent feature while still not enabling GCC extensions, you need
  //to define the below macro AT THE TOP OF THE FILE to allow platform-specific headers to be exposed.
  #define _DEFAULT_SOURCE 1
#endif 

#include "recs.h"
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>




//returns number of ticks per nanosecond of time for the respective OS and machine.
static int64_t nanoseconds_per_tick(void);

//get timestamp with nano-second resolution
static int64_t get_time_ns(int64_t ns_per_tick);


#if defined(_WIN32)
  #include <windows.h>

  static int64_t nanoseconds_per_tick(void) {
    LARGE_INTEGER ticks_per_second;
    assert(QueryPerformanceFrequency(&ticks_per_second));
    int64_t ts = ticks_per_second.QuadPart;

    //convert ticks per second to ticks per nanosecond
    return ts / (int64_t)1000000000;
  }

  static int64_t get_time_ns(int64_t ns_per_tick) {

    LARGE_INTEGER ticks;
    assert(QueryPerformanceCounter(&ticks));
    
    int64_t t = ticks.QuadPart; //QuadPart can be used directly on 64-bit systems
    return t / ns_per_tick;

  }


#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)

  #include <unistd.h>
  #include <time.h>


  //check if this OS is POSIX compliant by checking if
  //there's a defined version number. Works on MacOS and Linux
  #if defined(_POSIX_VERSION)

    static int64_t nanoseconds_per_tick(void) {
      struct timespec ts;
      assert(clock_getres(CLOCK_MONOTONIC, &ts) == 0);

      return (int64_t)ts.tv_nsec + ((int64_t)ts.tv_sec) * (int64_t)1000000000;
    }

    static int64_t get_time_ns(int64_t ns_per_tick) {
      struct timespec ts;

      //returns 0 on success, non-zero on failure
      assert(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);

      return (int64_t)ts.tv_nsec + ((int64_t)ts.tv_sec) * (int64_t)1000000000;


    }
  #else 
    #error "This platform does not support this tool for high performance benchmarking"

  #endif

#else 
#error "This platform does not support this tool for high performance benchmarking"

#endif




#define MAX_ENTITIES 1000
#define NUM_TICKS 100








#define GAME_WIDTH 540
#define GAME_HEIGHT 720 

#define BORDER_HEIGHT (50)
#define MAX_SYSTEM_GROUPS 1
#define TICK_STEP 1.0f/60.0f



#define PLAY_X (0)
#define PLAY_Y (BORDER_HEIGHT)
#define PLAY_WIDTH (GAME_WIDTH)
#define PLAY_HEIGHT (GAME_HEIGHT - 2*BORDER_HEIGHT)



typedef struct {
  float x;
  float y;
} Vector2;

typedef struct  {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

} Color;



enum system_group {
  SYSTEM_GROUP_UPDATE,
};



struct collider {
  float circle_radius;
};


enum component_type {
  COMP_TYPE_POSITION,
  COMP_TYPE_VEL,
  COMP_TYPE_COLLISION,

  //marks end of comp list, not an actual component
  COMP_TYPE_COUNT
};

//note that 
enum tag {
  TAG_A,     
  TAG_B,


  //marks end of tag list
  TAG_COUNT
};


struct component_position {
  Vector2 pos;
};



struct component_velocity {
  Vector2 vel;
};


typedef uint8_t collision_layers;
typedef collision_layers collision_mask;




#define COLLISION_LAYER_NONE ((collision_layers) 0)               // 0000 0000
#define COLLISION_LAYER_A ((collision_layers) 1 << 0)             // 0000 0001
#define COLLISION_MASK_NONE ((collision_mask) 0)                 // 0000 0000





struct component_collision {
  struct collider collider;

  // the collision layers that the component resides in
  collision_layers layers;

  // the collision layers that this component interacts with
  collision_mask mask;

  uint32_t num_collisions;


};



static inline Vector2 Vector2Add(const Vector2 a, const Vector2 b) {
  return (Vector2) {
    .x = a.x + b.x,
    .y = b.y + b.y
  };
}



static inline Vector2 Vector2Scale(const Vector2 a, const float b) {
  return (Vector2) {
    .x = a.x * b,
    .y = a.y * b
  };
}




static uint8_t collision_occurred(const Vector2 c1_pos, const struct collider *c1, const Vector2 c2_pos, const struct collider *c2) {
  float a = c1_pos.x - c2_pos.x;
  float b = c1_pos.y - c2_pos.y;

  float dis_sqrd = a*a + b*b;
  float rad = c1->circle_radius + c2->circle_radius;

  return dis_sqrd < rad*rad;

}



static recs_entity spawn_entity(recs ecs, Vector2 pos, Vector2 vel, uint32_t collision_mask, uint32_t collision_layer) {
  struct component_position p = {
    .pos = pos
  };

  struct component_velocity v = {
    .vel = vel
  };

  struct component_collision c = {
    .collider = {.circle_radius = 10},
    .layers = collision_layer,
    .mask = collision_mask,
    .num_collisions = 0
  };


  recs_entity e = recs_entity_add(ecs);

  recs_entity_add_component(ecs, e, COMP_TYPE_POSITION, &p);
  recs_entity_add_component(ecs, e, COMP_TYPE_VEL, &v);
  recs_entity_add_component(ecs, e, COMP_TYPE_COLLISION, &c);

  return e;
}


static void system_move_velocity(recs ecs) {
  float delta = TICK_STEP;
  uint8_t mask[RECS_GET_BITMASK_SIZE(COMP_TYPE_COUNT, TAG_COUNT)];
  recs_bitmask_create(ecs, mask, RECS_BITMASK_CREATE_COMP_ARG(2, COMP_TYPE_POSITION, COMP_TYPE_VEL), 0, NULL);
  recs_ent_iter iter = recs_ent_iter_init(ecs, mask);
  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);

    
    struct component_velocity *vel = (struct component_velocity*)recs_entity_get_component(ecs, e, COMP_TYPE_VEL);
    struct component_position *pos = (struct component_position*) recs_entity_get_component(ecs, e, COMP_TYPE_POSITION);

    Vector2 final_vel = Vector2Scale(vel->vel, delta);

    pos->pos = Vector2Add(pos->pos, final_vel);


  }
}




static inline void system_collision_process_colliders(recs ecs, struct component_position *p1, struct component_collision *c1, struct component_position *p2, struct component_collision *c2) {

  Vector2 pos1 = p1->pos;
  Vector2 pos2 = p2->pos;

  uint8_t collided = collision_occurred(pos1, &c1->collider, pos2, &c2->collider);

  if(!collided) return;


  uint8_t entity_1_can_hit_2 = (c2->layers & c1->mask) != 0;
  uint8_t entity_2_can_hit_1 = (c1->layers & c2->mask) != 0;


  if(entity_1_can_hit_2) {
    c1->num_collisions++;
  }

  if(entity_2_can_hit_1) {
    c2->num_collisions++;
  }


}



static void system_collision(recs ecs) {
  uint8_t mask[RECS_GET_BITMASK_SIZE(COMP_TYPE_COUNT, TAG_COUNT)];

  
  recs_bitmask_create(ecs, mask, RECS_BITMASK_CREATE_COMP_ARG(2, COMP_TYPE_COLLISION, COMP_TYPE_POSITION), 0, NULL);
  recs_ent_iter iter = recs_ent_iter_init_with_match(ecs, mask, RECS_ENT_MATCH_ALL);


  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);
    struct component_collision *c1 = recs_entity_get_component(ecs, e, COMP_TYPE_COLLISION);
    struct component_position *p1 = (struct component_position*) recs_entity_get_component(ecs, e, COMP_TYPE_POSITION);



    recs_ent_iter i2 = iter;

    while(recs_ent_iter_has_next(&i2)) {
      recs_entity e2 = recs_ent_iter_next(ecs, &i2);

      struct component_collision *c2 = recs_entity_get_component(ecs, e2, COMP_TYPE_COLLISION);
      struct component_position *p2 = (struct component_position*) recs_entity_get_component(ecs, e2, COMP_TYPE_POSITION);

      system_collision_process_colliders(ecs, p1, c1, p2, c2);

    }
  }
}



static inline recs ecs_init(void) {
  static struct recs_init_config_component ECS_COMPONENT_LIST[COMP_TYPE_COUNT] = {
    {.type = COMP_TYPE_POSITION, .comp_size = sizeof(struct component_position), .max_components = MAX_ENTITIES},
    {.type = COMP_TYPE_VEL, .comp_size = sizeof(struct component_velocity), .max_components = MAX_ENTITIES},
    {.type = COMP_TYPE_COLLISION, .comp_size = sizeof(struct component_collision), .max_components = MAX_ENTITIES},

  };

  static struct recs_init_config_system ECS_SYSTEM_LIST[] = {
    {.func=system_move_velocity, .group=SYSTEM_GROUP_UPDATE},
    {.func=system_collision, .group=SYSTEM_GROUP_UPDATE},
  };

  #define MAX_SYSTEMS (sizeof(ECS_SYSTEM_LIST) / sizeof(struct recs_init_config_system))




  //init ECS
  struct recs_init_config config = {
    .context = NULL,
    .max_entities = MAX_ENTITIES,
    .max_systems = MAX_SYSTEMS,
    .max_system_groups = MAX_SYSTEM_GROUPS,
    .max_tags = TAG_COUNT,
    .max_component_types = COMP_TYPE_COUNT,
    .systems = ECS_SYSTEM_LIST,
    .components = ECS_COMPONENT_LIST,
  };

  return recs_init(config);
}


static float get_random_float(float min, float max) {
  uint32_t r = rand();

  //normalize to between 0 and 1
  float n = (float) r / (float) RAND_MAX;

  float diff = max - min;
  return min + n*diff;
}


static inline Vector2 get_random_vec2(const Vector2 min, const Vector2 max) {
  return (Vector2) {
    .x = get_random_float(min.x, max.x),
    .y = get_random_float(min.y, max.y)
  };
}




static inline void ecs_prepare(recs ecs) {
  //add entities

  const Vector2 pos_min = {.x = PLAY_X, .y = PLAY_Y};
  const Vector2 pos_max = {.x = PLAY_X + PLAY_WIDTH, .y = PLAY_Y + PLAY_HEIGHT};

  const Vector2 vec_min = {.x = -2, .y = -2};
  const Vector2 vec_max = {.x = 2, .y = 2};



  //spawn entities
  for(uint32_t i = 0; i < MAX_ENTITIES; i++) {
    spawn_entity(
      ecs, 
      get_random_vec2(pos_min, pos_max),
      get_random_vec2(vec_min, vec_max),
      COLLISION_LAYER_A,
      COLLISION_LAYER_A
    );
  }
}




static void run(recs ecs, uint32_t num_ticks) {
  for(uint32_t i = 0; i < num_ticks; i++) {
    recs_system_run(ecs, SYSTEM_GROUP_UPDATE);
  }  
  
}

int main(void) {

  recs ecs = ecs_init();
  if(ecs == NULL) {
    printf("Cannot allocate\n");
    return 1;
  }

  ecs_prepare(ecs);

  int64_t ns_per_tick = nanoseconds_per_tick();
  
  printf("Clock Resolution (nanoseconds per clock tick) %" PRId64 "\n", ns_per_tick);
  printf("\n");


  int64_t ns_before = get_time_ns(ns_per_tick);
  run(ecs, NUM_TICKS);
  int64_t ns_after = get_time_ns(ns_per_tick);

  recs_free(ecs);


  int64_t ns_diff = ns_after - ns_before;
  int64_t ns = ns_diff;

  int64_t s = ns / 1000000000;
  ns -= s * 1000000000;

  int64_t ms = ns / 1000000;
  ns -= ms * 1000000;

  int64_t mcs = ns / 1000;
  ns -= mcs * 1000;

  printf("It took \n");
  printf("\t%" PRId64 " seconds,\n"  , s);
  printf("\t%" PRId64 " milliseconds,\n", ms);
  printf("\t%" PRId64 " microseconds,\n", mcs);
  printf("\t%" PRId64 " nanoseconds,\n", ns);
  printf("to run the game!\n\n");



}

