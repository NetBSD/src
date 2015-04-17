#	$NetBSD: bsd.arm-lpae.mk,v 1.1 2015/04/17 20:13:51 matt Exp $

.ifndef _BSD_ARM_LPAE_MK_
_BSD_ARM_LPAE_MK_=1

KMODULEARCHDIR:=	arm-lpae

# gcc emits bad code with these options
CPPFLAGS+=	-DARM_HAS_LPAE

.endif # _BSD_ARM_LPAE_MK_
