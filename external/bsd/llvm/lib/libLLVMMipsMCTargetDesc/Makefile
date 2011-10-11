#	$NetBSD: Makefile,v 1.2 2011/10/11 13:53:59 joerg Exp $

LIB=	LLVMMipsMCTargetDesc

.include <bsd.init.mk>

MIPS_OBJDIR!=	cd ${.CURDIR}/../libLLVMMipsCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${MIPS_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/Mips

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips/MCTargetDesc

SRCS+=	MipsAsmBackend.cpp \
	MipsMCAsmInfo.cpp \
	MipsMCCodeEmitter.cpp \
	MipsMCTargetDesc.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
