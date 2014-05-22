#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 16:14:46 yamt Exp $

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
