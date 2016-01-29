#include <stdio.h>
#include <bfd_stdint.h>

extern void foo (void);
extern void check_ptr_eq (void *, void *);

void
new_foo (void)
{
}

__asm__(".symver new_foo, foo@@VERS_2.0");

#if defined(__GNUC__) && (__GNUC__ * 1000 + __GNUC_MINOR__) >= 4005
__attribute__ ((noinline, noclone))
#else
__attribute__ ((noinline))
#endif
int
bar (void)
{
  return (intptr_t) &foo == 0x12345678 ? 1 : 0;
}

int
main(void)
{
  bar ();
  check_ptr_eq (&foo, &new_foo);
  printf("PASS\n");
  return 0;
}
