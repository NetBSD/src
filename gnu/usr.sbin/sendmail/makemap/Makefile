#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:35 tron Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/makemap ${DIST}/sendmail/src

PROG=		makemap

SRCS=		makemap.c safefile.c
MAN=		makemap.8
CPPFLAGS+=	-I${DIST}/sendmail/src ${DBMDEF} -DNOT_SENDMAIL

.include <bsd.prog.mk>
