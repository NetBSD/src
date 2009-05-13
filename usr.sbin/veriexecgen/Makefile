# $NetBSD: Makefile,v 1.2.26.1 2009/05/13 19:20:43 jym Exp $

WARNS?=	2	# XXX -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

PROG=	veriexecgen
MAN=	veriexecgen.8

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
