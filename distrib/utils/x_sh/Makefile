# 	$NetBSD: Makefile,v 1.14 2001/08/20 12:19:46 martin Exp $
# Build a smaller sh (e.g. for boot media)

.include <bsd.own.mk>

MKMAN=no

SRCDIR= ${.CURDIR}/../../../bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf ${SRCDIR}/../../bin/test

.include "${SRCDIR}/Makefile"
