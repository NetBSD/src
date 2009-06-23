/*	$NetBSD: gccw.c,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

 /*
  * This is is a regression test for all the things that gcc is meant to warn
  * about.
  * 
  * gcc version 3 breaks several tests:
  * 
  * -W does not report missing return value
  * 
  * -Wunused does not report unused parameter
  */

#include <stdio.h>
#include <setjmp.h>

jmp_buf jbuf;

 /* -Wmissing-prototypes: no previous prototype for 'test1' */
 /* -Wimplicit: return type defaults to `int' */
test1(void)
{
    /* -Wunused: unused variable `foo' */
    int     foo;

    /* -Wparentheses: suggest parentheses around && within || */
    printf("%d\n", 1 && 2 || 3 && 4);
    /* -W: statement with no effect */
    0;
    /* BROKEN in gcc 3 */
    /* -W: control reaches end of non-void function */
}


 /* -W??????: unused parameter `foo' */
void    test2(int foo)
{
    enum {
    a = 10, b = 15} moe;
    int     bar;

    /* -Wuninitialized: 'bar' might be used uninitialized in this function */
    /* -Wformat: format argument is not a pointer (arg 2) */
    printf("%s\n", bar);
    /* -Wformat: too few arguments for format */
    printf("%s%s\n", "bar");
    /* -Wformat: too many arguments for format */
    printf("%s\n", "bar", "bar");

    /* -Wswitch: enumeration value `b' not handled in switch */
    switch (moe) {
    case a:
	return;
    }
}

 /* -Wstrict-prototypes: function declaration isn't a prototype */
void    test3()
{
}
