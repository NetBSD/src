/*	$NetBSD: fmtcheck.c,v 1.2 2022/04/14 15:51:29 martin Exp $	*/
#include <stdio.h>
const char *
fmtcheck(const char *a, const char *b)
{
	return a ? a : b;
}
