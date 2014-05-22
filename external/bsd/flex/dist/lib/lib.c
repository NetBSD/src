/*	$NetBSD: lib.c,v 1.1.1.1.10.2 2014/05/22 15:45:07 yamt Exp $	*/

/* Since building an empty library could cause problems, we provide a
 * function to go into the library. We could make this non-trivial by
 * moving something that flex treats as a library function into this
 * directory. */

void do_nothing(){ return;}

