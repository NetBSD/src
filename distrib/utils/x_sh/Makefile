# 	$NetBSD: Makefile,v 1.11 2000/04/11 12:45:18 christos Exp $
# Build a smaller sh (e.g. for boot media)

.include <bsd.own.mk>

MKMAN=no

SRCDIR= ${.CURDIR}/../../../bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf ${SRCDIR}/../../bin/test

.include "${SRCDIR}/Makefile"
