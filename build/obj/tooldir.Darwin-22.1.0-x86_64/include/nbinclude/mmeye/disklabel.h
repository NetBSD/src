/*	$NetBSD: disklabel.h,v 1.4 2011/08/30 12:39:56 bouyer Exp $	*/

#define LABELUSESMBR	1
#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sh3/disklabel.h>
#else
#include <sh3/disklabel.h>
#endif /* HAVE_NBTOOL_CONFIG_H */
