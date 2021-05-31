#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:06 cjep Exp $

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
