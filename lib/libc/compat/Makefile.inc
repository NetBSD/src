#	$NetBSD: Makefile.inc,v 1.1 2005/09/13 01:44:08 christos Exp $

# compat sources

CPPFLAGS+=-I${NETBSDSRCDIR}/sys

.include "${.CURDIR}/compat/db/Makefile.inc"
.include "${.CURDIR}/compat/gen/Makefile.inc"
.include "${.CURDIR}/compat/stdlib/Makefile.inc"
.include "${.CURDIR}/compat/sys/Makefile.inc"

# XXX: Conditional until all archs are converted.
.if exists(${.CURDIR}/compat/arch/${ARCHSUBDIR}/Makefile.inc)
COMPATARCHDIR=${.CURDIR}/compat/arch/${ARCHSUBDIR}
.PATH: ${COMPATARCHDIR}/gen ${COMPATARCHDIR}/sys
.include "${.CURDIR}/compat/arch/${ARCHSUBDIR}/Makefile.inc"
.endif
