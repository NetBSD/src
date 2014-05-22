#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:48 yamt Exp $

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
