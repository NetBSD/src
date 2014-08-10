#	$NetBSD: Makefile,v 1.1 2014/08/10 17:34:21 joerg Exp $

LIB=	LLVMMCAnalysis

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC/MCAnalysis

SRCS+=	MCAtom.cpp \
	MCFunction.cpp \
	MCModule.cpp \
	MCModuleYAML.cpp \
	MCObjectDisassembler.cpp \
	MCObjectSymbolizer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
