/*	$NetBSD: errno.c,v 1.3 2000/02/13 21:13:01 kleink Exp $	*/

#include <sys/cdefs.h>

__warn_references(errno,
    "warning: reference to deprecated errno; include <errno.h> for correct reference")

int errno;
