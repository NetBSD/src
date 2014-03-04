#	$NetBSD: Makefile,v 1.10 2014/03/04 21:05:06 joerg Exp $

LIB=	LLVMMipsMCTargetDesc

.include <bsd.init.mk>

MIPS_OBJDIR!=	cd ${.CURDIR}/../libLLVMMipsCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${MIPS_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/Mips

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips/MCTargetDesc

SRCS+=	MipsAsmBackend.cpp \
	MipsELFObjectWriter.cpp \
	MipsMCAsmInfo.cpp \
	MipsMCCodeEmitter.cpp \
	MipsMCExpr.cpp \
	MipsMCTargetDesc.cpp \
	MipsNaClELFStreamer.cpp \
	MipsTargetStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
