/* Simple program using the JIT API.  */

#include <stdint.h>

struct jit_code_entry
{
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
};

struct jit_descriptor
{
  uint32_t version;
  /* This type should be jit_actions_t, but we use uint32_t
     to be explicit about the bitwidth.  */
  uint32_t action_flag;
  struct jit_code_entry *relevant_entry;
  struct jit_code_entry *first_entry;
};

#ifdef SPACER
/* This exists to change the address of __jit_debug_descriptor.  */
int spacer = 4;
#endif

struct jit_descriptor __jit_debug_descriptor = { 1, 0, 0, 0 };

void __jit_debug_register_code()
{
}

int main()
{
  return 0;
}
