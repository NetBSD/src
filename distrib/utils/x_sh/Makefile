# 	$NetBSD: Makefile,v 1.16 2001/12/12 00:05:14 tv Exp $
# Build a smaller sh (e.g. for boot media)

NOMAN=		# defined

.include <bsd.own.mk>

SRCDIR=		${.CURDIR}/../../../bin/sh

CPPFLAGS+=	-I${SRCDIR} -DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf ${SRCDIR}/../../bin/test

.include "${SRCDIR}/Makefile"
