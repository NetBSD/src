#	$NetBSD: Makefile,v 1.11 2002/04/21 20:22:12 bjh21 Exp $

# Don't hook up testfloat just yet. (ross, 2001.3.12)

SUBDIR+= infinity round
# FP exceptions are broken on NetBSD/arm
.if ${MACHINE_ARCH} != "arm"
SUBDIR+= except
.endif
.if ${MACHINE} != "vax"
SUBDIR+= nan
.endif


.include <bsd.subdir.mk>
