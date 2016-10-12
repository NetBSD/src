#include "list0.h"

int main ()
{
    int x;

    x = 0;
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    foo (x++);
    return 0;
}

static void
unused ()
{
    /* Not used for anything */
} /* last line */
