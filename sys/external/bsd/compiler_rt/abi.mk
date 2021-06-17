# $NetBSD: abi.mk,v 1.1.2.2 2021/06/17 04:46:32 thorpej Exp $

.if !empty(MACHINE_ARCH:Mearm*hf*)
CPPFLAGS+=	-DCOMPILER_RT_ARMHF_TARGET
.endif
