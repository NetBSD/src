#	$NetBSD: Makefile,v 1.1 2011/07/05 17:22:22 joerg Exp $

LIB=	LLVMARMDisassembler

.include <bsd.init.mk>

ARM_OBJDIR!=	cd ${.CURDIR}/../libLLVMARMCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${ARM_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/ARM

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM/Disassembler

SRCS+=	ARMDisassembler.cpp \
	ARMDisassemblerCore.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
