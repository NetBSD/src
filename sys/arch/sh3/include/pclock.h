/*	$NetBSD: pclock.h,v 1.1 2002/02/01 17:52:56 uch Exp $	*/

#include "opt_pclock.h"

extern int sh3_pclock;	/* defined in sh3/sh3_machdep.c */
#ifndef PCLOCK
#define PCLOCK sh3_pclock
#endif
