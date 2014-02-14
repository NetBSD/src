#	$NetBSD: Makefile,v 1.9 2014/02/14 20:23:56 joerg Exp $

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
	MipsTargetStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
