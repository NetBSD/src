#	$NetBSD: Makefile,v 1.16 2018/07/17 18:55:27 joerg Exp $

LIB=	LLVMAsmPrinter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/AsmPrinter

SRCS+=	AccelTable.cpp \
	AddressPool.cpp \
	ARMException.cpp \
	AsmPrinter.cpp \
	AsmPrinterDwarf.cpp \
	AsmPrinterInlineAsm.cpp \
	CodeViewDebug.cpp \
	DbgValueHistoryCalculator.cpp \
	DebugHandlerBase.cpp \
	DebugLocStream.cpp \
	DIE.cpp \
	DIEHash.cpp \
	DwarfCFIException.cpp \
	DwarfCompileUnit.cpp \
	DwarfDebug.cpp \
	DwarfExpression.cpp \
	DwarfFile.cpp \
	DwarfStringPool.cpp \
	DwarfUnit.cpp \
	EHStreamer.cpp \
	ErlangGCPrinter.cpp \
	OcamlGCPrinter.cpp \
	WinCFGuard.cpp \
	WinException.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
