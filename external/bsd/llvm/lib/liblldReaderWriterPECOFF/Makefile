#	$NetBSD: Makefile,v 1.3.4.2 2014/05/22 16:14:47 yamt Exp $

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
