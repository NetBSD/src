#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:29 tls Exp $

LIB=	lldbPluginObjectFilePECOFF

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/ObjectFile/PECOFF

SRCS+=	ObjectFilePECOFF.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
