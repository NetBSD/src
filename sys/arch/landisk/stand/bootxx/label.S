/*	$NetBSD: label.S,v 1.1.2.2 2006/09/03 15:23:20 yamt Exp $	*/

#include <machine/asm.h>

	.text
	.global	_C_LABEL(ptn_disklabel)
_C_LABEL(ptn_disklabel):
	.fill	512
