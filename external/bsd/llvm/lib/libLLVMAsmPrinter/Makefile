#	$NetBSD: Makefile,v 1.11 2014/05/30 18:28:14 joerg Exp $

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
	DwarfException.cpp \
	DwarfFile.cpp \
	DwarfStringPool.cpp \
	DwarfUnit.cpp \
	ErlangGCPrinter.cpp \
	OcamlGCPrinter.cpp \
	Win64Exception.cpp \
	WinCodeViewLineTables.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
