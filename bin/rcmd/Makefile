#	$NetBSD: Makefile,v 1.6 2002/08/19 09:56:01 lukem Exp $

.include <bsd.own.mk>

PROG=	rcmd
SRCS=	rsh.c
CPPFLAGS+=-DIN_RCMD
BINOWN=	root
BINMODE=4555
.PATH:	${NETBSDSRCDIR}/usr.bin/rsh

.include <bsd.prog.mk>
