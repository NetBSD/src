#	$NetBSD: Makefile,v 1.3 1998/03/29 09:22:06 christos Exp $
# Build a smaller sh (e.g. for boot media)

NOMAN=1
SRCDIR=${.CURDIR}/../../../bin/sh

CPPFLAGS+=	-I${SRCDIR}
CFLAGS+=	-DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf
.include "${SRCDIR}/Makefile"
