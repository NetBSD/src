# 	$NetBSD: Makefile,v 1.15 2001/10/05 01:17:32 jmc Exp $
# Build a smaller sh (e.g. for boot media)

.include <bsd.own.mk>

MKMAN=no

SRCDIR= ${.CURDIR}/../../../bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf ${SRCDIR}/../../bin/test

.include "${SRCDIR}/Makefile"
