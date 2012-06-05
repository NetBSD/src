#	$NetBSD: Makefile,v 1.1.10.1 2012/06/05 21:15:46 bouyer Exp $

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

.include <bsd.prog.mk>
