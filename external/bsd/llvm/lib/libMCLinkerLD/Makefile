#	$NetBSD: Makefile,v 1.2.12.2 2014/05/22 16:14:44 yamt Exp $

LIB=	MCLinkerLD

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/LD

SRCS=	Archive.cpp \
	ArchiveReader.cpp \
	BranchIsland.cpp \
	BranchIslandFactory.cpp \
	BSDArchiveReader.cpp \
	Diagnostic.cpp \
	DiagnosticEngine.cpp \
	DiagnosticInfos.cpp \
	DiagnosticLineInfo.cpp \
	DiagnosticPrinter.cpp \
	DWARFLineInfo.cpp \
	DynObjReader.cpp \
	EhFrame.cpp \
	EhFrameHdr.cpp \
	EhFrameReader.cpp \
	ELFBinaryReader.cpp \
	ELFDynObjFileFormat.cpp \
	ELFDynObjReader.cpp \
	ELFExecFileFormat.cpp \
	ELFFileFormat.cpp \
	ELFObjectReader.cpp \
	ELFObjectWriter.cpp \
	ELFReader.cpp \
	ELFReaderIf.cpp \
	ELFSegment.cpp \
	ELFSegmentFactory.cpp \
	GNUArchiveReader.cpp \
	GroupReader.cpp \
	LDContext.cpp \
	LDFileFormat.cpp \
	LDReader.cpp \
	LDSection.cpp \
	LDSymbol.cpp \
	MsgHandler.cpp \
	NamePool.cpp \
	ObjectWriter.cpp \
	RelocationFactory.cpp \
	Relocator.cpp \
	RelocData.cpp \
	ResolveInfo.cpp \
	Resolver.cpp \
	SectionData.cpp \
	SectionRules.cpp \
	SectionSymbolSet.cpp \
	StaticResolver.cpp \
	StubFactory.cpp \
	TextDiagnosticPrinter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
