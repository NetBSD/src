# $NetBSD: Makefile.inc,v 1.1.12.3 2013/06/23 06:21:03 tls Exp $

CPPFLAGS+=	-DELFSIZE=32
.if (!empty(CPUFLAGS) && ${CPUFLAGS:M-mabi=aapcs*} != "")
CPPFLAGS+=      -DHAVE_INITFINI_ARRAY
.endif
