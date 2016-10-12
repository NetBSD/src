/* An include file that actually causes code to be generated in the including file.  This is known to cause problems on some systems. */

extern void bar(int);
static void foo (int x)
/* !
   !
   ! */
{
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
}
