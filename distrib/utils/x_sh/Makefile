# 	$NetBSD: Makefile,v 1.12 2000/05/18 00:43:08 matt Exp $
# Build a smaller sh (e.g. for boot media)

.include <bsd.own.mk>

MKMAN=no

SRCDIR= ${.CURDIR}/../../../bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL
COPTS+= -Os

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf ${SRCDIR}/../../bin/test

.include "${SRCDIR}/Makefile"
