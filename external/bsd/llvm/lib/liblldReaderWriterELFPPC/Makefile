#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:27 tls Exp $

LIB=	lldReaderWriterELFPPC

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/PPC

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	PPCLinkingContext.cpp \
	PPCTargetHandler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
