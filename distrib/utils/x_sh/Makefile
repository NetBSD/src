#	$NetBSD: Makefile,v 1.4 1998/04/13 14:10:05 lukem Exp $
# Build a smaller sh (e.g. for boot media)

NOMAN=1
SRCDIR=${.CURDIR}/../../../bin/sh

CPPFLAGS+=	-I${SRCDIR} -DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf
.include "${SRCDIR}/Makefile"
