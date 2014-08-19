# $NetBSD: Makefile.inc,v 1.1.12.4 2014/08/20 00:02:06 tls Exp $

CPPFLAGS+=	-I${ARCHDIR}
.if (!empty(CPUFLAGS) && ${CPUFLAGS:M-mabi=aapcs*} != "")
CPPFLAGS+=      -DHAVE_INITFINI_ARRAY
.endif
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${CSU_MACHINE_ARCH}\"
