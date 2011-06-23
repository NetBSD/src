#	$NetBSD: Makefile,v 1.4.2.1 2011/06/23 14:18:30 cherry Exp $

LIB=	LLVMAsmPrinter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/AsmPrinter

SRCS+=	ARMException.cpp \
	AsmPrinter.cpp \
	AsmPrinterDwarf.cpp \
	AsmPrinterInlineAsm.cpp \
	DIE.cpp \
	DwarfCFIException.cpp \
	DwarfCompileUnit.cpp \
	DwarfDebug.cpp \
	DwarfException.cpp \
	OcamlGCPrinter.cpp \
	Win64Exception.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
