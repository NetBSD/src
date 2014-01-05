#	$NetBSD: Makefile,v 1.2 2014/01/05 16:38:07 joerg Exp $

LIB=	lldReaderWriterPECOFF

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/PECOFF

SRCS+=	EdataPass.cpp \
	IdataPass.cpp \
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
