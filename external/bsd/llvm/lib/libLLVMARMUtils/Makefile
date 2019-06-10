#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 21:50:51 christos Exp $

LIB=	LLVMARMUtils

.include <bsd.init.mk>

ARM_OBJDIR!=	cd ${.CURDIR}/../libLLVMARMCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${ARM_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/ARM

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM/Utils

SRCS+=	ARMBaseInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
