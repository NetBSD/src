#	$NetBSD: Makefile,v 1.10 1999/02/12 12:45:13 tv Exp $

PROG=		diff3
SRCS=		diff3.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I${.CURDIR}/../diff -I${DIST}/diffutils \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

DIST=		${.CURDIR}/../../dist
.PATH: ${.CURDIR}/../diff ${DIST}/diffutils

.include <bsd.prog.mk>
