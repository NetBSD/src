#	$NetBSD: Makefile,v 1.1.6.1 2024/02/29 12:28:01 martin Exp $

.include <bsd.init.mk>

PROG=	nslookup
SRCS=	nslookup.c dighost.c
MAN=	nslookup.8
BINDIR=	/usr/bin

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

.PATH: ${DIST} ${IDIST}/doc/man
DPADD+=${LIBEDIT} ${LIBTERMINFO}
LDADD+=-ledit -lterminfo

nslookup.o: ${VERSIONFILE}

.include <bsd.prog.mk>
