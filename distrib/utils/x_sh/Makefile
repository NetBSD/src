# 	$NetBSD: Makefile,v 1.10 1999/06/21 01:19:38 cgd Exp $
# Build a smaller sh (e.g. for boot media)

.include <bsd.own.mk>

MKMAN=no

SRCDIR= ${.CURDIR}/../../../bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf

.include "${SRCDIR}/Makefile"
