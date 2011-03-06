#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:46 bouyer Exp $

LIB=	LLVMCppBackendInfo

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/CppBackend

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/CppBackend/TargetInfo

SRCS+=	CppBackendTargetInfo.cpp

.include <bsd.lib.mk>
