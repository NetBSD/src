#	$NetBSD: Makefile,v 1.5.4.1 2015/06/04 20:05:44 snj Exp $

LIB=	LLVMDebugInfo

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo

SRCS+=	DIContext.cpp \
	DWARFAbbreviationDeclaration.cpp \
	DWARFAcceleratorTable.cpp \
	DWARFCompileUnit.cpp \
	DWARFContext.cpp \
	DWARFDebugAbbrev.cpp \
	DWARFDebugArangeSet.cpp \
	DWARFDebugAranges.cpp \
	DWARFDebugFrame.cpp \
	DWARFDebugInfoEntry.cpp \
	DWARFDebugLine.cpp \
	DWARFDebugLoc.cpp \
	DWARFDebugRangeList.cpp \
	DWARFFormValue.cpp \
	DWARFTypeUnit.cpp \
	DWARFUnit.cpp \
	SyntaxHighlighting.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
