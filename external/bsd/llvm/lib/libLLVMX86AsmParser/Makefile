#	$NetBSD: Makefile,v 1.2.8.1 2014/08/10 07:09:09 tls Exp $

LIB=	LLVMX86AsmParser

.include <bsd.init.mk>

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/X86

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/AsmParser

SRCS+=	X86AsmParser.cpp \
	X86AsmInstrumentation.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
