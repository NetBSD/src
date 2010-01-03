# $NetBSD: Makefile,v 1.4 2010/01/03 18:27:10 christos Exp $

NOMAN=1
PROG=wtc1
CPPFLAGS=-I${.CURDIR}/..
LDADD+=-ledit -ltermcap
DPADD+=${LIBEDIT} ${LIBTERMCAP}

.ifdef DEBUG
CPPFLAGS+=-DDEBUG
.endif

.include <bsd.prog.mk>
