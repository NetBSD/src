#	$NetBSD: Makefile,v 1.6.2.1 2004/11/12 10:37:56 he Exp $

.include <bsd.own.mk>

PROG=	rcmd
SRCS=	rsh.c getport.c
CPPFLAGS+=-DIN_RCMD
BINOWN=	root
BINMODE=4555
.PATH:	${NETBSDSRCDIR}/usr.bin/rsh

.include <bsd.prog.mk>
