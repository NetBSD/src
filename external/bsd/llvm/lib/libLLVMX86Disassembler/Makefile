#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:04 joerg Exp $

LIB=	LLVMX86Disassembler

.include <bsd.init.mk>

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR}

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/Disassembler

SRCS+=	X86Disassembler.cpp \
	X86DisassemblerDecoder.c

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
