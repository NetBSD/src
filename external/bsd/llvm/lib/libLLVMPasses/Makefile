#	$NetBSD: Makefile,v 1.1.14.1 2018/07/28 04:37:11 pgoyette Exp $

LIB=	LLVMPasses

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Passes

SRCS+=	PassBuilder.cpp \
	PassPlugin.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
