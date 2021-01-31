/*	$NetBSD: d_cast_typeof.c,v 1.3 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_cast_typeof.c"

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
