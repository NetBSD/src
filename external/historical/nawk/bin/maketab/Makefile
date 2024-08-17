#	$NetBSD: Makefile,v 1.1 2024/08/17 22:22:29 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

.include <bsd.hostinit.mk>

DIST = ${.CURDIR}/../../dist
.PATH: ${DIST}

PROG=	maketab
SRCS=	maketab.c
YHEADER=yes
CPPFLAGS+=	-I.

maketab.c: awkgram.h
awkgram.h: awkgram.y
CLEANFILES+= awkgram.c awkgram.h

proginstall:

.include <bsd.prog.mk>
