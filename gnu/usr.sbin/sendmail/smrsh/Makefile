#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:36 tron Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/smrsh

PROG=		smrsh

MAN=		smrsh.8
CPPFLAGS+=	-I${DIST}/sendmail/src

.include <bsd.prog.mk>
