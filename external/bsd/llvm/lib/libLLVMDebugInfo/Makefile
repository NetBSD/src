#	$NetBSD: Makefile,v 1.1 2011/10/11 13:53:59 joerg Exp $

LIB=	LLVMDebugInfo

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo

SRCS+=	DIContext.cpp \
	DWARFAbbreviationDeclaration.cpp \
	DWARFCompileUnit.cpp \
	DWARFContext.cpp \
	DWARFDebugAbbrev.cpp \
	DWARFDebugArangeSet.cpp \
	DWARFDebugAranges.cpp \
	DWARFDebugInfoEntry.cpp \
	DWARFDebugLine.cpp \
	DWARFFormValue.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
