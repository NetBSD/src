# $NetBSD: Makefile.inc,v 1.5 2013/04/29 07:21:59 skrll Exp $

CPPFLAGS+=	-DELFSIZE=32
.if (!empty(CPUFLAGS) && ${CPUFLAGS:M-mabi=aapcs*} != "")
CPPFLAGS+=      -DHAVE_INITFINI_ARRAY
.endif
