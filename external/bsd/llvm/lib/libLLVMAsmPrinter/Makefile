#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:53 joerg Exp $

LIB=	LLVMAsmPrinter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/AsmPrinter

SRCS+=	AsmPrinter.cpp \
	AsmPrinterDwarf.cpp \
	AsmPrinterInlineAsm.cpp \
	DIE.cpp \
	DwarfCFIException.cpp \
	DwarfDebug.cpp \
	DwarfException.cpp \
	DwarfTableException.cpp \
	OcamlGCPrinter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
