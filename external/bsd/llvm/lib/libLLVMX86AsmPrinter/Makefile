#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:02 joerg Exp $

LIB=	LLVMX86AsmPrinter
NOGCCERROR=	yes

.include <bsd.init.mk>

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR}

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/InstPrinter

SRCS+=	X86ATTInstPrinter.cpp \
	X86IntelInstPrinter.cpp \
	X86InstComments.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
