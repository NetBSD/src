/*	$NetBSD: label.S,v 1.1 2006/09/01 21:26:19 uwe Exp $	*/

#include <machine/asm.h>

	.text
	.global	_C_LABEL(ptn_disklabel)
_C_LABEL(ptn_disklabel):
	.fill	512
