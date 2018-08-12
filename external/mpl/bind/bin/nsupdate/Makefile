#	$NetBSD: Makefile,v 1.1 2018/08/12 13:02:26 christos Exp $

.include <bsd.own.mk>

PROG=	nsupdate
MAN=	nsupdate.1
BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/nsupdate

.PATH: ${DIST}
DPADD+=${LIBEDIT} ${LIBTERMINFO}
LDADD+=-ledit -lterminfo

nsupdate.o: ${VERSIONFILE}

.include <bsd.prog.mk>
