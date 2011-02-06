#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:55 joerg Exp $

LIB=	LLVMCBackendInfo

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/CBackend

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/CBackend/TargetInfo

SRCS+=	CBackendTargetInfo.cpp

.include <bsd.lib.mk>
