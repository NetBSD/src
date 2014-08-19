#	$NetBSD: Makefile.macppc.inc,v 1.1.14.1 2014/08/20 00:03:11 tls Exp $

.if ${PPC_ARCH_MODE} == "ppc64bridge"
AOPTS+=	${${ACTIVE_CC} == "gcc":? -Wa,-mppc64bridge :}
.endif

AFLAGS+=	${AOPTS}
