#include "vers.h"

int
bar ()
{
  return 3;
}

#pragma weak hide_original_foo

int
hide_original_foo ()
{
  return 1 + bar ();
}

#pragma weak hide_old_foo

int
hide_old_foo ()
{
  return 10 + bar();
}

#pragma weak hide_old_foo1

int
hide_old_foo1 ()
{
  return 100 + bar ();
}

#pragma weak hide_new_foo

int
hide_new_foo ()
{
  return 1000 + bar ();
}

SYMVER(hide_original_foo, show_foo@);
SYMVER(hide_old_foo, show_foo@VERS_1.1);
SYMVER(hide_old_foo1, show_foo@VERS_1.2);
SYMVER(hide_new_foo, show_foo@@VERS_2.0);
