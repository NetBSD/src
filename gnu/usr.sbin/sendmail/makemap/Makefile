#	$NetBSD: Makefile,v 1.2 2000/04/05 10:29:37 erh Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/makemap ${DIST}/sendmail/src

PROG=		makemap

SRCS=		makemap.c safefile.c snprintf.c
MAN=		makemap.8
CPPFLAGS+=	-I${DIST}/sendmail/src ${DBMDEF} -DNOT_SENDMAIL

.include <bsd.prog.mk>
