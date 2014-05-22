#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:47 yamt Exp $

LIB=	lldReaderWriterNative

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/Native

SRCS+=	ReaderNative.cpp \
	WriterNative.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
