#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:50 bouyer Exp $

LIB=	LLVMX86AsmParser

.include <bsd.init.mk>

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/X86

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/AsmParser

SRCS+=	X86AsmLexer.cpp \
	X86AsmParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
