#	$NetBSD: Makefile,v 1.1.4.2 2014/08/19 23:51:28 tls Exp $

LIB=	lldbPluginDynamicLoaderHexagonDYLD

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/DynamicLoader/Hexagon-DYLD

SRCS+=	DynamicLoaderHexagonDYLD.cpp \
	HexagonDYLDRendezvous.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
