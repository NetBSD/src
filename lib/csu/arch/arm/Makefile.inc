# $NetBSD: Makefile.inc,v 1.9 2018/12/28 18:17:10 christos Exp $

CPPFLAGS+=	-I${ARCHDIR}
.if (!empty(CPUFLAGS) && ${CPUFLAGS:M-mabi=aapcs*} != "")
CPPFLAGS+=      -DHAVE_INITFINI_ARRAY
.endif
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${CSU_MACHINE_ARCH}\"
