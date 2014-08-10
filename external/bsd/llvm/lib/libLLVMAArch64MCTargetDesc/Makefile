#	$NetBSD: Makefile,v 1.1.10.1 2014/08/10 07:09:08 tls Exp $

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
	AArch64MCTargetDesc.cpp \
	AArch64MachObjectWriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
