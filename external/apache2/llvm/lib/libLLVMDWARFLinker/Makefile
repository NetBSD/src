#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:51 joerg Exp $

LIB=	LLVMDWARFLinker

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DWARFLinker

SRCS+=	DWARFLinker.cpp \
	DWARFLinkerCompileUnit.cpp \
	DWARFLinkerDeclContext.cpp \
	DWARFStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
