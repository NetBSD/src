/*	$NetBSD: errno.c,v 1.2.12.1 2000/02/15 10:05:06 he Exp $	*/

#include <sys/cdefs.h>

__warn_references(errno,
    "warning: reference to deprecated errno; include <errno.h> for correct reference")

int errno;
