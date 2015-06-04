    #	$NetBSD: Makefile,v 1.3.2.1 2015/06/04 20:05:46 snj Exp $

LIB=	lldReaderWriterELFMips

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/Mips

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	MipsCtorsOrderPass.cpp \
	MipsELFFlagsMerger.cpp \
	MipsLinkingContext.cpp \
	MipsRelocationHandler.cpp \
	MipsRelocationPass.cpp \
	MipsTargetHandler.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
