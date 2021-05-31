#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:06 cjep Exp $

LIB=	LLVMAsmPrinter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/AsmPrinter

SRCS+=	AccelTable.cpp \
	AddressPool.cpp \
	AIXException.cpp \
	ARMException.cpp \
	AsmPrinter.cpp \
	AsmPrinterDwarf.cpp \
	AsmPrinterInlineAsm.cpp \
	CodeViewDebug.cpp \
	DbgEntityHistoryCalculator.cpp \
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
	PseudoProbePrinter.cpp \
	WasmException.cpp \
	WinCFGuard.cpp \
	WinException.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
