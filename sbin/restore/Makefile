#	$NetBSD: Makefile,v 1.17 1997/05/27 08:45:57 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	restore
LINKS=	${BINDIR}/restore ${BINDIR}/rrestore
CFLAGS+=-DRRESTORE
SRCS=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c \
	dumprmt.c
BINGRP=	tty
BINMODE=2555
MAN=	restore.8
MLINKS+=restore.8 rrestore.8
.PATH:	${.CURDIR}/../dump

.include <bsd.prog.mk>
