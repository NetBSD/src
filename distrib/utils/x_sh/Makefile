# 	$NetBSD: Makefile,v 1.13 2000/05/18 01:10:31 matt Exp $
# Build a smaller sh (e.g. for boot media)

.include <bsd.own.mk>

MKMAN=no

SRCDIR= ${.CURDIR}/../../../bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL
COPTS+= -Os
LDSTATIC?=-static

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf ${SRCDIR}/../../bin/test

.include "${SRCDIR}/Makefile"
