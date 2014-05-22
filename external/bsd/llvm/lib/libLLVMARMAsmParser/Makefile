#	$NetBSD: Makefile,v 1.1.2.1 2014/05/22 16:14:41 yamt Exp $

LIB=	LLVMARMAsmParser

.include <bsd.init.mk>

ARM_OBJDIR!=	cd ${.CURDIR}/../libLLVMARMCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${ARM_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/ARM

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM/AsmParser

SRCS+=	ARMAsmParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
