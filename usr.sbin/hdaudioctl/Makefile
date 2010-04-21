# $NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:27:21 matt Exp $

PROG=		hdaudioctl
SRCS=		hdaudioctl.c
SRCS+=		graph.c
MAN=		hdaudioctl.8

LDADD+=		-lprop
DPADD+=		${LIBPROP}

.include <bsd.prog.mk>
