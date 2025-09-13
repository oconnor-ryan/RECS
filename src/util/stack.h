#ifndef STACK_H
#define STACK_H

#include <stdint.h>

struct stack {
  char *buffer;
  uint32_t max_elements;
  uint32_t element_size;

  uint32_t num_elements;
  uint32_t top_of_stack;
};

void stack_init(struct stack *s, uint32_t element_size, char *buffer, uint32_t max_elements);

int stack_push(struct stack *s, void *element);
int stack_pop(struct stack *s, void *element);



#endif// STACK_H
