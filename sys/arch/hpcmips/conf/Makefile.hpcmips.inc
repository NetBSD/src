#	$NetBSD: Makefile.hpcmips.inc,v 1.1.28.1 2004/04/07 04:51:08 jmc Exp $

MIPSFLAGS?=	-mips2
CFLAGS+=	${MIPSFLAGS}
AFLAGS+=	${MIPSFLAGS}
