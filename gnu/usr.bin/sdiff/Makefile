#	$NetBSD: Makefile,v 1.11 2000/11/01 14:34:15 wiz Exp $

PROG=		sdiff
SRCS=		sdiff.c version.c
CPPFLAGS+=	-I${.CURDIR}/../diff -I${DIST}/diffutils \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

DIST=		${.CURDIR}/../../dist
.PATH: ${.CURDIR}/../diff ${DIST}/diffutils

.include <bsd.prog.mk>
