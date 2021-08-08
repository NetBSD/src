# $NetBSD: abi.mk,v 1.1.8.2 2021/08/08 10:11:39 martin Exp $

.if !empty(MACHINE_ARCH:Mearm*hf*)
CPPFLAGS+=	-DCOMPILER_RT_ARMHF_TARGET
.endif
