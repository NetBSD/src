/*	$NetBSD: d_cast_typeof.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_cast_typeof.c"

/* lint1-extra-flags: -X 351 */

struct foo {
	char list;
};
struct foo *hole;

char *
foo(void)
{
	return hole ?
	    ((char *)&((typeof(hole))0)->list) :
	    ((char *)&((typeof(*hole) *)0)->list);
}
