#	$NetBSD: Makefile.macppc.inc,v 1.1.2.2 2011/06/23 14:19:19 cherry Exp $

.if ${PPC_ARCH_MODE} == "ppc64bridge"
AOPTS+=	-Wa,-mppc64bridge
.endif

CFLAGS+=	-Wa,-maltivec
AFLAGS+=	${AOPTS}
