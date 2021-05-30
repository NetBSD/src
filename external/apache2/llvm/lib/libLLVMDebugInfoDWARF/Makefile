#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:51 joerg Exp $

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
	DWARFDebugAddr.cpp \
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
	DWARFListTable.cpp \
	DWARFLocationExpression.cpp \
	DWARFTypeUnit.cpp \
	DWARFUnit.cpp \
	DWARFUnitIndex.cpp \
	DWARFVerifier.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
