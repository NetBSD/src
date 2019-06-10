#	$NetBSD: Makefile,v 1.3.16.1 2019/06/10 21:50:55 christos Exp $

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
