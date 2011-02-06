#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:59 joerg Exp $

LIB=	LLVMMC

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC

SRCS+=	ELFObjectWriter.cpp \
	MCAsmInfo.cpp \
	MCAsmInfoCOFF.cpp \
	MCAsmInfoDarwin.cpp \
	MCAsmStreamer.cpp \
	MCAssembler.cpp \
	MCCodeEmitter.cpp \
	MCContext.cpp \
	MCDisassembler.cpp \
	MCELFObjectTargetWriter.cpp \
	MCELFStreamer.cpp \
	MCExpr.cpp \
	MCInst.cpp \
	MCInstPrinter.cpp \
	MCLabel.cpp \
	MCDwarf.cpp \
	MCLoggingStreamer.cpp \
	MCMachOStreamer.cpp \
	MCMachObjectTargetWriter.cpp \
	MCNullStreamer.cpp \
	MCObjectStreamer.cpp \
	MCObjectWriter.cpp \
	MCPureStreamer.cpp \
	MCSection.cpp \
	MCSectionCOFF.cpp \
	MCSectionELF.cpp \
	MCSectionMachO.cpp \
	MCStreamer.cpp \
	MCSymbol.cpp \
	MCValue.cpp \
	MachObjectWriter.cpp \
	WinCOFFStreamer.cpp \
	WinCOFFObjectWriter.cpp \
	TargetAsmBackend.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
