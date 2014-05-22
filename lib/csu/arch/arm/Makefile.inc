# $NetBSD: Makefile.inc,v 1.1.6.2 2014/05/22 11:36:43 yamt Exp $

CPPFLAGS+=	-I${ARCHDIR}
.if (!empty(CPUFLAGS) && ${CPUFLAGS:M-mabi=aapcs*} != "")
CPPFLAGS+=      -DHAVE_INITFINI_ARRAY
.endif
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${CSU_MACHINE_ARCH}\"
