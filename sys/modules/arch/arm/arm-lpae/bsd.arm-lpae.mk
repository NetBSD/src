#	$NetBSD: bsd.arm-lpae.mk,v 1.1.2.2 2015/06/06 14:40:23 skrll Exp $

.ifndef _BSD_ARM_LPAE_MK_
_BSD_ARM_LPAE_MK_=1

KMODULEARCHDIR:=	arm-lpae

# gcc emits bad code with these options
CPPFLAGS+=	-DARM_HAS_LPAE

.endif # _BSD_ARM_LPAE_MK_
