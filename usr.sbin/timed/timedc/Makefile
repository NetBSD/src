#	$NetBSD: Makefile,v 1.8 1997/10/17 08:57:03 mrg Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/93

PROG=	timedc
SRCS=	cmds.c cmdtab.c timedc.c byteorder.c measure.c cksum.c
MAN=	timedc.8
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../timed

.include "../../Makefile.inc"
.include <bsd.prog.mk>
