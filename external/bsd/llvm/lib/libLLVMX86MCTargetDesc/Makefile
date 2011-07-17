#	$NetBSD: Makefile,v 1.3 2011/07/17 20:07:59 joerg Exp $

LIB=	LLVMX86MCTargetDesc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/MCTargetDesc

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/X86

SRCS+=	X86MCAsmInfo.cpp \
	X86MCTargetDesc.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
