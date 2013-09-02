#	$NetBSD: Makefile,v 1.1 2013/09/02 14:34:57 joerg Exp $

LIB=	lldReaderWriterMachO

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/MachO

SRCS+=	MachOLinkingContext.cpp \
	ReferenceKinds.cpp \
	WriterMachO.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
