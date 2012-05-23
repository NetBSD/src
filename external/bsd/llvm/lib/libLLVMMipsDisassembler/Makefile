#	$NetBSD: Makefile,v 1.1.2.2 2012/05/23 10:07:27 yamt Exp $

LIB=	LLVMMipsDisassembler

.include <bsd.init.mk>

MIPS_OBJDIR!=	cd ${.CURDIR}/../libLLVMMipsCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${MIPS_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/Mips

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips/Disassembler

SRCS+=	MipsDisassembler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
