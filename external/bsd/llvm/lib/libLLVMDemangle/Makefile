#	$NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:56:00 pgoyette Exp $

LIB=	LLVMDemangle

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Demangle

SRCS+=	ItaniumDemangle.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
