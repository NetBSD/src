    #	$NetBSD: Makefile,v 1.1.2.1 2014/08/10 07:09:10 tls Exp $

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
