#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:47 yamt Exp $

LIB=	lldReaderWriterYAML

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/YAML

SRCS+=	ReaderWriterYAML.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
