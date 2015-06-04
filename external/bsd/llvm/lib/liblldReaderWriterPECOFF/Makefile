#	$NetBSD: Makefile,v 1.3.6.1 2015/06/04 20:05:46 snj Exp $

LIB=	lldReaderWriterPECOFF

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/PECOFF

SRCS+=	EdataPass.cpp \
	IdataPass.cpp \
	LinkerGeneratedSymbolFile.cpp \
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
