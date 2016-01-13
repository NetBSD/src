/*	$NetBSD: xmalloc.c,v 1.1.1.1 2016/01/13 18:41:48 christos Exp $	*/

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

char *xmalloc(int n)
{
    return XtMalloc(n);
}
