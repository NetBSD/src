#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:28 tls Exp $

LIB=	lldbLog

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source

SRCS+=	lldb.cpp \
	lldb-log.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
