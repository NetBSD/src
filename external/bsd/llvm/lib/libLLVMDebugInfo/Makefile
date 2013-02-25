#	$NetBSD: Makefile,v 1.1.8.2 2013/02/25 00:26:38 tls Exp $

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
	DWARFDebugFrame.cpp \
	DWARFDebugInfoEntry.cpp \
	DWARFDebugLine.cpp \
	DWARFDebugRangeList.cpp \
	DWARFFormValue.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
