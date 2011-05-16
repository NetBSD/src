#	$NetBSD: Makefile,v 1.4 2011/05/16 19:27:48 joerg Exp $

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
	OcamlGCPrinter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
