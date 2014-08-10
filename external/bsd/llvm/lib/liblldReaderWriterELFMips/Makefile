    #	$NetBSD: Makefile,v 1.3 2014/08/10 17:35:07 joerg Exp $

LIB=	lldReaderWriterELFMips

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/Mips

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	MipsCtorsOrderPass.cpp \
	MipsLinkingContext.cpp \
	MipsRelocationHandler.cpp \
	MipsRelocationPass.cpp \
	MipsTargetHandler.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
