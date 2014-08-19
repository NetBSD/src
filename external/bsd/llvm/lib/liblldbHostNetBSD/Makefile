#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:28 tls Exp $

LIB=	lldbHostNetBSD

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Host/freebsd

SRCS+=	Host.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
