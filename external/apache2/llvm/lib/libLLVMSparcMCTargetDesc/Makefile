#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:12 joerg Exp $

LIB=	LLVMSparcMCTargetDesc

.include <bsd.init.mk>

SPARC_OBJDIR!=	cd ${.CURDIR}/../libLLVMSparcCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${SPARC_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/Sparc

.PATH: ${LLVM_SRCDIR}/lib/Target/Sparc/MCTargetDesc

SRCS+=	SparcAsmBackend.cpp \
	SparcELFObjectWriter.cpp \
	SparcInstPrinter.cpp \
	SparcMCAsmInfo.cpp \
	SparcMCCodeEmitter.cpp \
	SparcMCExpr.cpp \
	SparcMCTargetDesc.cpp \
	SparcTargetStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
