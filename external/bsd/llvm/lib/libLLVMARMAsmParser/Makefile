#	$NetBSD: Makefile,v 1.1 2011/07/05 17:22:22 joerg Exp $

LIB=	LLVMARMAsmParser

.include <bsd.init.mk>

ARM_OBJDIR!=	cd ${.CURDIR}/../libLLVMARMCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${ARM_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/ARM

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM/AsmParser

SRCS+=	ARMAsmLexer.cpp \
	ARMAsmParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
