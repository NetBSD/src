#	$NetBSD: Makefile,v 1.3 1997/07/20 20:43:53 christos Exp $

WARNS=	1
PROG=	rcmd
SRCS=	rsh.c
CFLAGS+=-DIN_RCMD
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../../usr.bin/rsh

.include <bsd.prog.mk>
