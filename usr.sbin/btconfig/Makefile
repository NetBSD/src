# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:56 gdamore Exp $

PROG=		btconfig
SRCS=		btconfig.c
MAN=		btconfig.8

DPADD+=		${LIBBLUETOOTH} ${LIBUTIL}
LDADD+=		-lbluetooth -lutil

.include <bsd.prog.mk>
