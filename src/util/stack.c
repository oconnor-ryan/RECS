#include "stack.h"
#include <string.h>


void stack_init(struct stack *s, uint32_t element_size, char *buffer, uint32_t max_elements) {
  s->buffer = buffer;
  s->element_size = element_size;
  s->max_elements = max_elements;
  s->num_elements = 0;
  s->top_of_stack = 0;
}

int stack_push(struct stack *s, void *element) {
  if(s->num_elements >= s->max_elements) return 0;

  char *top_of_stack = s->buffer + (s->element_size * s->num_elements);

  memcpy(top_of_stack, element, s->element_size);
  s->num_elements++;

  return 1;
}

int stack_pop(struct stack *s, void *element) {
  if(s->num_elements == 0) return 0;

  char *top_of_stack = s->buffer + (s->element_size * (s->num_elements - 1));
  memcpy(element, top_of_stack, s->element_size);
  s->num_elements--;

  return 1;
}
