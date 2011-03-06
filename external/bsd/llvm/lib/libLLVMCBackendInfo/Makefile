#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:45 bouyer Exp $

LIB=	LLVMCBackendInfo

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/CBackend

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/CBackend/TargetInfo

SRCS+=	CBackendTargetInfo.cpp

.include <bsd.lib.mk>
