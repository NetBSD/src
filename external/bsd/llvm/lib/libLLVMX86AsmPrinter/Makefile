#	$NetBSD: Makefile,v 1.4 2018/07/17 18:55:29 joerg Exp $

LIB=	LLVMX86AsmPrinter

.include <bsd.init.mk>

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/X86

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/InstPrinter

SRCS+=	X86ATTInstPrinter.cpp \
	X86InstComments.cpp \
	X86InstPrinterCommon.cpp \
	X86IntelInstPrinter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
