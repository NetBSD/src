#	$NetBSD: Makefile,v 1.5 2014/05/30 18:28:15 joerg Exp $

LIB=	lldReaderWriterMachO

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/MachO

SRCS+=	MachOLinkingContext.cpp \
	MachONormalizedFileBinaryReader.cpp \
	MachONormalizedFileBinaryWriter.cpp \
	MachONormalizedFileFromAtoms.cpp \
	MachONormalizedFileToAtoms.cpp \
	MachONormalizedFileYAML.cpp \
	ReferenceKinds.cpp \
	WriterMachO.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
