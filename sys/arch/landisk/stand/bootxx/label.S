/*	$NetBSD: label.S,v 1.1.12.2 2006/12/30 20:46:24 yamt Exp $	*/

#include <machine/asm.h>

	.text
	.global	_C_LABEL(ptn_disklabel)
_C_LABEL(ptn_disklabel):
	.fill	512
