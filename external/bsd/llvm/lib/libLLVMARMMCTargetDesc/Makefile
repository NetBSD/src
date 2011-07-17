#	$NetBSD: Makefile,v 1.2 2011/07/17 20:07:58 joerg Exp $

LIB=	LLVMARMMCTargetDesc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM/MCTargetDesc

ARM_OBJDIR!=	cd ${.CURDIR}/../libLLVMARMCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${ARM_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/ARM

SRCS+=	ARMMCAsmInfo.cpp \
	ARMMCTargetDesc.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
