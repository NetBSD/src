#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:47 yamt Exp $

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
