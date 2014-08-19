#	$NetBSD: Makefile,v 1.6.4.2 2014/08/19 23:51:27 tls Exp $

LIB=	lldReaderWriterMachO

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/MachO

SRCS+=	ArchHandler_x86.cpp \
	ArchHandler_x86_64.cpp \
	ArchHandler_arm.cpp \
	ArchHandler.cpp \
	GOTPass.cpp \
	MachOLinkingContext.cpp \
	MachONormalizedFileBinaryReader.cpp \
	MachONormalizedFileBinaryWriter.cpp \
	MachONormalizedFileFromAtoms.cpp \
	MachONormalizedFileToAtoms.cpp \
	MachONormalizedFileYAML.cpp \
	StubsPass.cpp \
	WriterMachO.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
