#	$NetBSD: Makefile,v 1.5 1997/10/10 19:56:46 christos Exp $

PROG=	rcmd
SRCS=	rsh.c
CPPFLAGS+=-DIN_RCMD
BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../../usr.bin/rsh

.include <bsd.prog.mk>
