    #	$NetBSD: Makefile,v 1.2 2014/05/30 18:28:15 joerg Exp $

LIB=	lldReaderWriterELFMips

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/Mips

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	MipsLinkingContext.cpp \
	MipsRelocationHandler.cpp \
	MipsRelocationPass.cpp \
	MipsTargetHandler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
