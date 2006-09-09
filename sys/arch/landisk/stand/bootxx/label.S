/*	$NetBSD: label.S,v 1.1.6.2 2006/09/09 02:40:57 rpaulo Exp $	*/

#include <machine/asm.h>

	.text
	.global	_C_LABEL(ptn_disklabel)
_C_LABEL(ptn_disklabel):
	.fill	512
