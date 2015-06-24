#	$NetBSD: Makefile,v 1.2 2015/06/24 22:20:25 matt Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.X11}/src/util

PROG?=		makekeys
SRCS=		makekeys.c
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
