#	$NetBSD: Makefile,v 1.2.12.1 2016/11/05 17:54:06 martin Exp $

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
