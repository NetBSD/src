#	$NetBSD: Makefile.macppc.inc,v 1.2 2014/08/10 17:44:26 joerg Exp $

.if ${PPC_ARCH_MODE} == "ppc64bridge"
AOPTS+=	${${ACTIVE_CC} == "gcc":? -Wa,-mppc64bridge :}
.endif

AFLAGS+=	${AOPTS}
