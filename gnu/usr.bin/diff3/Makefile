#	$NetBSD: Makefile,v 1.12 2000/11/01 14:34:14 wiz Exp $

PROG=		diff3
SRCS=		diff3.c version.c
CPPFLAGS+=	-I${.CURDIR}/../diff -I${DIST}/diffutils \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

DIST=		${.CURDIR}/../../dist
.PATH: ${.CURDIR}/../diff ${DIST}/diffutils

.include <bsd.prog.mk>
