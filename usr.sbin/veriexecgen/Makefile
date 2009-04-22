# $NetBSD: Makefile,v 1.3 2009/04/22 15:23:09 lukem Exp $

WARNS?=	2	# XXX -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

PROG=	veriexecgen
MAN=	veriexecgen.8

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
