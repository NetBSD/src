#	$NetBSD: Makefile.hpcmips.inc,v 1.2 2004/04/06 16:37:58 tv Exp $

MIPSFLAGS?=	-mips2
CFLAGS+=	${MIPSFLAGS}
AFLAGS+=	${MIPSFLAGS}
