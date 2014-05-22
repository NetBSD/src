#	$NetBSD: Makefile,v 1.1.8.2 2014/05/22 16:14:43 yamt Exp $

LIB=	LLVMPowerPCAsmParser

.include <bsd.init.mk>

PPC_OBJDIR!=	cd ${.CURDIR}/../libLLVMPowerPCCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${PPC_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/PowerPC

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC/AsmParser

SRCS+=	PPCAsmParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
