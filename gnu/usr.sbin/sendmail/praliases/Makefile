#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:35 tron Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/praliases

PROG=		praliases

MAN=		praliases.8
CPPFLAGS+=	-I${DIST}/sendmail/src ${DBMDEF}

.include <bsd.prog.mk>
