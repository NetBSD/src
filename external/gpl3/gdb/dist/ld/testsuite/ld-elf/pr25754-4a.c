#include <stdio.h>
#include <bfd_stdint.h>

extern uintptr_t *get_bar (void);

int
main ()
{
  if ((uintptr_t) get_bar () == 0x7fffffffULL)
    printf ("PASS\n");
  return 0;
}
