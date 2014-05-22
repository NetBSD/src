#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:43 yamt Exp $

LIB=	LLVMSparcAsmPrinter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/Sparc/InstPrinter

SPARC_OBJDIR!=	cd ${.CURDIR}/../libLLVMSparcCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${SPARC_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/Sparc

SRCS+=	SparcInstPrinter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
