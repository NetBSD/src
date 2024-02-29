#	$NetBSD: Makefile,v 1.1.6.1 2024/02/29 12:28:01 martin Exp $

.include <bsd.init.mk>

PROG=	nsupdate
MAN=	nsupdate.1
BINDIR=	/usr/bin

DIST=${IDIST}/bin/nsupdate

.PATH: ${DIST} ${IDIST}/doc/man
DPADD+=${LIBEDIT} ${LIBTERMINFO}
LDADD+=-ledit -lterminfo

.include <bsd.prog.mk>
