#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:45 bouyer Exp $

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
