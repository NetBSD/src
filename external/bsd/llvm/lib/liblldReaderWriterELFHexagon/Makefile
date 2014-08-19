#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:27 tls Exp $

LIB=	lldReaderWriterELFHexagon

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/Hexagon

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	HexagonLinkingContext.cpp \
	HexagonRelocationHandler.cpp \
	HexagonTargetHandler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
