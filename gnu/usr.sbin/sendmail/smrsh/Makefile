#	$NetBSD: Makefile,v 1.2 2000/02/13 21:32:01 veego Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/smrsh

PROG=		smrsh

MAN=		smrsh.8
CPPFLAGS+=	-I${DIST}/sendmail/src

BINDIR=		/usr/libexec

.include <bsd.prog.mk>
