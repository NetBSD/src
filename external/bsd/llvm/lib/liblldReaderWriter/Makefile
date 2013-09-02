#	$NetBSD: Makefile,v 1.1 2013/09/02 14:34:57 joerg Exp $

LIB=	lldReaderWriter

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter

SRCS+=	CoreLinkingContext.cpp \
	LinkerScript.cpp \
	Reader.cpp \
	ReaderArchive.cpp \
	ReaderLinkerScript.cpp \
	Writer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
