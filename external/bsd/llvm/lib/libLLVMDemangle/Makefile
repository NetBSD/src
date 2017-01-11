#	$NetBSD: Makefile,v 1.1 2017/01/11 12:02:29 joerg Exp $

LIB=	LLVMDemangle

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Demangle

SRCS+=	ItaniumDemangle.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
