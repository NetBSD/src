#	$NetBSD: Makefile,v 1.12 2000/11/08 00:08:30 wiz Exp $

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I${.CURDIR}/../diff -I${DIST}/diffutils \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

DIST=		${.CURDIR}/../../dist
.PATH: ${.CURDIR}/../diff ${DIST}/diffutils

.include <bsd.prog.mk>
