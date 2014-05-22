#	$NetBSD: Makefile,v 1.1.2.2 2014/05/22 16:14:41 yamt Exp $

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
	DWARFDebugLoc.cpp \
	DWARFDebugRangeList.cpp \
	DWARFFormValue.cpp \
	DWARFTypeUnit.cpp \
	DWARFUnit.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
