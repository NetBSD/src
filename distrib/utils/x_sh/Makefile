# 	$NetBSD: Makefile,v 1.9 1999/05/19 04:17:50 gwr Exp $
# Build a smaller sh (e.g. for boot media)

.include <bsd.own.mk>

MKMAN=no

SRCDIR= ${BSDSRCDIR}/bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf

.include "${SRCDIR}/Makefile"
