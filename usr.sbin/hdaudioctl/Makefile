# $NetBSD: Makefile,v 1.1 2009/10/11 08:57:54 sborrill Exp $

PROG=		hdaudioctl
SRCS=		hdaudioctl.c
SRCS+=		graph.c
MAN=		hdaudioctl.8

LDADD+=		-lprop
DPADD+=		${LIBPROP}

.include <bsd.prog.mk>
