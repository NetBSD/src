#	$NetBSD: Makefile,v 1.3 2014/03/04 21:05:06 joerg Exp $

LIB=	lldReaderWriterPECOFF

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/PECOFF

SRCS+=	EdataPass.cpp \
	IdataPass.cpp \
	LoadConfigPass.cpp \
	PECOFFLinkingContext.cpp \
	Pass.cpp \
	ReaderCOFF.cpp \
	ReaderImportHeader.cpp \
	WriterImportLibrary.cpp \
	WriterPECOFF.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
