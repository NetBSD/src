#include <unistd.h>

struct foo_t
{
  int foo;
  size_t bar;
  const char *baz;
  struct foo_t *self;
  const volatile union
  {
    double should_not_appear;
    char *nor_should_this;
  } named;
  volatile const struct
  {
    long unnamed_sub_member;
    union
    {
      double one_more_level;
      long yes_really_one_more;
    };
    struct {
      int and_finally;
    };
  };
  struct {};		/* Empty ones */
  union {};
  int after_the_end;
};

struct foo_t used;
