#	$NetBSD: Makefile,v 1.2.8.2 2014/08/19 23:51:27 tls Exp $

LIB=	lldReaderWriterELF

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	ArrayOrderPass.cpp \
	ELFLinkingContext.cpp \
	Reader.cpp \
	Writer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
