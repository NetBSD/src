#	$NetBSD: Makefile,v 1.15 1997/05/06 20:45:26 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	restore
LINKS=	${BINDIR}/restore ${BINDIR}/rrestore
CFLAGS+=-DRRESTORE
SRCS=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c \
	dumprmt.c
BINOWN=	root
BINGRP=	tty
BINMODE=6555
MAN=	restore.8
MLINKS+=restore.8 rrestore.8
.PATH.c: ${.CURDIR}/../dump

.include <bsd.prog.mk>
