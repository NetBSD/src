#	$NetBSD: Makefile,v 1.6.2.1 2015/06/04 20:05:46 snj Exp $

LIB=	lldReaderWriterMachO

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/MachO

SRCS+=	ArchHandler_arm64.cpp \
	ArchHandler_x86.cpp \
	ArchHandler_x86_64.cpp \
	ArchHandler_arm.cpp \
	ArchHandler.cpp \
	CompactUnwindPass.cpp \
	GOTPass.cpp \
	MachOLinkingContext.cpp \
	MachONormalizedFileBinaryReader.cpp \
	MachONormalizedFileBinaryWriter.cpp \
	MachONormalizedFileFromAtoms.cpp \
	MachONormalizedFileToAtoms.cpp \
	MachONormalizedFileYAML.cpp \
	ShimPass.cpp \
	StubsPass.cpp \
	WriterMachO.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
