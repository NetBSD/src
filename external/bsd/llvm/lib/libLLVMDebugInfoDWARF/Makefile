#	$NetBSD: Makefile,v 1.4 2018/07/17 18:55:27 joerg Exp $

LIB=	LLVMDebugInfoDWARF

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/DWARF

SRCS+=	DWARFAbbreviationDeclaration.cpp \
	DWARFAcceleratorTable.cpp \
	DWARFAddressRange.cpp \
	DWARFCompileUnit.cpp \
	DWARFContext.cpp \
	DWARFDataExtractor.cpp \
	DWARFDebugAbbrev.cpp \
	DWARFDebugAranges.cpp \
	DWARFDebugArangeSet.cpp \
	DWARFDebugFrame.cpp \
	DWARFDebugInfoEntry.cpp \
	DWARFDebugLine.cpp \
	DWARFDebugLoc.cpp \
	DWARFDebugMacro.cpp \
	DWARFDebugPubTable.cpp \
	DWARFDebugRangeList.cpp \
	DWARFDebugRnglists.cpp \
	DWARFDie.cpp \
	DWARFExpression.cpp \
	DWARFFormValue.cpp \
	DWARFGdbIndex.cpp \
	DWARFTypeUnit.cpp \
	DWARFUnit.cpp \
	DWARFUnitIndex.cpp \
	DWARFVerifier.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
