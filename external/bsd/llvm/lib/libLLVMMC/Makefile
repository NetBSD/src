#	$NetBSD: Makefile,v 1.12 2013/05/28 17:37:34 joerg Exp $

LIB=	LLVMMC

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/MC

SRCS+=	ELFObjectWriter.cpp \
	MCAsmBackend.cpp \
	MCAsmInfo.cpp \
	MCAsmInfoCOFF.cpp \
	MCAsmInfoDarwin.cpp \
	MCAsmStreamer.cpp \
	MCAssembler.cpp \
	MCAtom.cpp \
	MCCodeGenInfo.cpp \
	MCCodeEmitter.cpp \
	MCContext.cpp \
	MCDisassembler.cpp \
	MCDwarf.cpp \
	MCELF.cpp \
	MCELFObjectTargetWriter.cpp \
	MCELFStreamer.cpp \
	MCExpr.cpp \
	MCExternalSymbolizer.cpp \
	MCFunction.cpp \
	MCInst.cpp \
	MCInstrAnalysis.cpp \
	MCInstPrinter.cpp \
	MCLabel.cpp \
	MCModule.cpp \
	MCMachOStreamer.cpp \
	MCMachObjectTargetWriter.cpp \
	MCNullStreamer.cpp \
	MCObjectDisassembler.cpp \
	MCObjectFileInfo.cpp \
	MCObjectStreamer.cpp \
	MCObjectSymbolizer.cpp \
	MCObjectWriter.cpp \
	MCPureStreamer.cpp \
	MCRegisterInfo.cpp \
	MCRelocationInfo.cpp \
	MCSection.cpp \
	MCSectionCOFF.cpp \
	MCSectionELF.cpp \
	MCSectionMachO.cpp \
	MCStreamer.cpp \
	MCSubtargetInfo.cpp \
	MCSymbolizer.cpp \
	MCSymbol.cpp \
	MCValue.cpp \
	MCWin64EH.cpp \
	MachObjectWriter.cpp \
	WinCOFFStreamer.cpp \
	WinCOFFObjectWriter.cpp \
	SubtargetFeature.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
