#	$NetBSD: Makefile,v 1.3 2000/02/13 22:14:28 veego Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/smrsh

BINDIR=		/usr/libexec
PROG=		smrsh

MAN=		smrsh.8
CPPFLAGS+=	-I${DIST}/sendmail/src
CPPFLAGS+=	-DCMDDIR=\"/usr/libexec/sm.bin\"

.include <bsd.prog.mk>
