#include <stdio.h>
#include <bfd_stdint.h>

extern intptr_t size;

int
main ()
{
  if (size == 293 /* size of this file */)
    printf ("PASS\n");
#ifdef __GLIBC_PREREQ
# if !__GLIBC_PREREQ (2, 28)
  else
    printf ("PASS (incorrect result due to glibc bug)\n");
# endif
#endif
  return 0;
}
