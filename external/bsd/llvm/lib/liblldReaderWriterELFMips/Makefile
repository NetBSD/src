#	$NetBSD: Makefile,v 1.1 2014/01/05 16:38:07 joerg Exp $

LIB=	lldReaderWriterELFMips

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/Mips

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	MipsLinkingContext.cpp \
	MipsRelocationHandler.cpp \
	MipsTargetHandler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
