#	$NetBSD: Makefile,v 1.3 2016/11/01 21:56:12 christos Exp $

.include <bsd.own.mk>

PROG=	nslookup
SRCS=	nslookup.c dighost.c
MAN=	nslookup.8
BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

.PATH: ${DIST}
DPADD+=${LIBEDIT} ${LIBTERMINFO}
LDADD+=-ledit -lterminfo

nslookup.o: ${VERSIONFILE}

.include <bsd.prog.mk>
