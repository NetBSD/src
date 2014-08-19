#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:27 tls Exp $

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
