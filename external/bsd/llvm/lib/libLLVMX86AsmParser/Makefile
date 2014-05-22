#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:14:44 yamt Exp $

LIB=	LLVMX86AsmParser

.include <bsd.init.mk>

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/X86

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/AsmParser

SRCS+=	X86AsmParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
