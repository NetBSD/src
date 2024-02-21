#	$NetBSD: Makefile,v 1.3 2024/02/21 22:50:56 christos Exp $

.include <bsd.init.mk>

PROG=	nsupdate
MAN=	nsupdate.1
BINDIR=	/usr/bin

DIST=${IDIST}/bin/nsupdate

.PATH: ${DIST} ${IDIST}/doc/man
DPADD+=${LIBEDIT} ${LIBTERMINFO}
LDADD+=-ledit -lterminfo

.include <bsd.prog.mk>
