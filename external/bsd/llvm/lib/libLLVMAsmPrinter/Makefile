#	$NetBSD: Makefile,v 1.12 2014/08/10 17:34:21 joerg Exp $

LIB=	LLVMAsmPrinter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/AsmPrinter

SRCS+=	ARMException.cpp \
	AddressPool.cpp \
	AsmPrinter.cpp \
	AsmPrinterDwarf.cpp \
	AsmPrinterInlineAsm.cpp \
	DIE.cpp \
	DIEHash.cpp \
	DbgValueHistoryCalculator.cpp \
	DwarfAccelTable.cpp \
	DwarfCFIException.cpp \
	DwarfDebug.cpp \
	DwarfFile.cpp \
	DwarfStringPool.cpp \
	DwarfUnit.cpp \
	EHStreamer.cpp \
	ErlangGCPrinter.cpp \
	OcamlGCPrinter.cpp \
	Win64Exception.cpp \
	WinCodeViewLineTables.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
