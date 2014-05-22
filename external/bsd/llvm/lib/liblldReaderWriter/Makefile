#	$NetBSD: Makefile,v 1.3.4.2 2014/05/22 16:14:46 yamt Exp $

LIB=	lldReaderWriter

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter

SRCS+=	CoreLinkingContext.cpp \
	FileArchive.cpp \
	LinkerScript.cpp \
	Reader.cpp \
	Writer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
