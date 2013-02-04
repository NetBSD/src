#	$NetBSD: Makefile,v 1.5 2013/02/04 15:00:34 joerg Exp $

LIB=	LLVMMipsMCTargetDesc

.include <bsd.init.mk>

MIPS_OBJDIR!=	cd ${.CURDIR}/../libLLVMMipsCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${MIPS_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/Mips

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips/MCTargetDesc

SRCS+=	MipsAsmBackend.cpp \
	MipsDirectObjLower.cpp \
	MipsELFObjectWriter.cpp \
	MipsELFStreamer.cpp \
	MipsMCAsmInfo.cpp \
	MipsMCCodeEmitter.cpp \
	MipsMCTargetDesc.cpp \
	MipsReginfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
