# $NetBSD: Makefile.inc,v 1.1.10.2 2013/01/22 22:56:35 matt Exp $

CPPFLAGS+=	-DELFSIZE=32
.if ${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb" \
    || ${CPUFLAGS:M-mabi=aapcs*} != ""
CPPFLAGS+=      -DHAVE_INITFINI_ARRAY
.endif
