/*	$NetBSD: fabs.S,v 1.2 1998/05/25 15:28:02 ws Exp $	*/

#include <machine/asm.h>

ENTRY(fabs)
	fabs	1,1
	blr
