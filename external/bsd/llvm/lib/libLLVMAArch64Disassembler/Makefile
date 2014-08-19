#	$NetBSD: Makefile,v 1.1.4.3 2014/08/19 23:51:23 tls Exp $

LIB=	LLVMAArch64Disassembler

.include <bsd.init.mk>

AARCH64_OBJDIR!=	cd ${.CURDIR}/../libLLVMAArch64CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${AARCH64_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/AArch64

.PATH: ${LLVM_SRCDIR}/lib/Target/AArch64/Disassembler

SRCS+=	AArch64Disassembler.cpp \
	AArch64ExternalSymbolizer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
