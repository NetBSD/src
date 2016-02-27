#	$NetBSD: Makefile,v 1.1 2016/02/27 21:47:33 joerg Exp $

LIB=	LLVMDebugInfoDWARF

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/DWARF

SRCS+=	DWARFAbbreviationDeclaration.cpp \
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
	DWARFDebugMacro.cpp \
	DWARFDebugRangeList.cpp \
	DWARFFormValue.cpp \
	DWARFTypeUnit.cpp \
	DWARFUnit.cpp \
	DWARFUnitIndex.cpp \
	SyntaxHighlighting.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
