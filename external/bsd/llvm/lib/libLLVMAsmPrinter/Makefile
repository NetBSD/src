#	$NetBSD: Makefile,v 1.5 2011/06/01 14:13:42 joerg Exp $

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
