#	$NetBSD: Makefile,v 1.1 2011/07/17 20:07:58 joerg Exp $

LIB=	LLVMMipsMCTargetDesc

.include <bsd.init.mk>

MIPS_OBJDIR!=	cd ${.CURDIR}/../libLLVMMipsCodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${MIPS_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/Mips

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips/MCTargetDesc

SRCS+=	MipsMCAsmInfo.cpp \
	MipsMCTargetDesc.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
