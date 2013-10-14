#	$NetBSD: Makefile,v 1.2 2013/10/14 01:37:40 joerg Exp $

LIB=	lldReaderWriterMachO

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/MachO

SRCS+=	MachOLinkingContext.cpp \
	MachONormalizedFileYAML.cpp \
	ReferenceKinds.cpp \
	WriterMachO.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
