#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:41 yamt Exp $

LIB=	LLVMAArch64MCTargetDesc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/AArch64/MCTargetDesc

AARCH64_OBJDIR!= cd ${.CURDIR}/../libLLVMAArch64CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${AARCH64_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/AArch64

SRCS+=	AArch64AsmBackend.cpp \
	AArch64ELFObjectWriter.cpp \
	AArch64ELFStreamer.cpp \
	AArch64MCAsmInfo.cpp \
	AArch64MCCodeEmitter.cpp \
	AArch64MCExpr.cpp \
	AArch64MCTargetDesc.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
