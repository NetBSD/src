/*	$NetBSD: xmalloc.c,v 1.1.1.1 2006/02/06 18:14:00 wiz Exp $	*/

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

char *xmalloc(int n)
{
    return XtMalloc(n);
}
