# $NetBSD: Makefile.inc,v 1.1.10.3 2013/01/23 02:14:54 matt Exp $

CPPFLAGS+=	-DELFSIZE=32
.if ${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb" \
    || (!empty(CPUFLAGS) && ${CPUFLAGS:M-mabi=aapcs*} != "")
CPPFLAGS+=      -DHAVE_INITFINI_ARRAY
.endif
