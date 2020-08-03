#	$NetBSD: Makefile,v 1.2 2020/08/03 17:23:36 christos Exp $

.include <bsd.init.mk>

PROG=	nsupdate
MAN=	nsupdate.1
BINDIR=	/usr/bin

DIST=${IDIST}/bin/nsupdate

.PATH: ${DIST} ${IDIST}/doc/man
DPADD+=${LIBEDIT} ${LIBTERMINFO}
LDADD+=-ledit -lterminfo

nsupdate.o: ${VERSIONFILE}

.include <bsd.prog.mk>
