#	$NetBSD: Makefile,v 1.2 2020/08/03 17:23:35 christos Exp $

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
