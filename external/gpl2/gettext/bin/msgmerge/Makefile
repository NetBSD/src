#	$NetBSD: Makefile,v 1.3 2016/01/15 18:03:32 tron Exp $

PROG=	msgmerge
SRCS=msgmerge.c msgl-fsearch.c

LDADD+=	-lm
DPADD+= ${LIBM}

COPTS.msgmerge.c = -Wno-stack-protector

.include "${.CURDIR}/../Makefile.inc.prog"
