/*	$NetBSD: pclock.h,v 1.1.2.2 2002/02/11 20:09:01 jdolecek Exp $	*/

#include "opt_pclock.h"

extern int sh3_pclock;	/* defined in sh3/sh3_machdep.c */
#ifndef PCLOCK
#define PCLOCK sh3_pclock
#endif
