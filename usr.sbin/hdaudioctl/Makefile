# $NetBSD: Makefile,v 1.1.2.2 2009/10/18 16:50:13 bouyer Exp $

PROG=		hdaudioctl
SRCS=		hdaudioctl.c
SRCS+=		graph.c
MAN=		hdaudioctl.8

LDADD+=		-lprop
DPADD+=		${LIBPROP}

.include <bsd.prog.mk>
