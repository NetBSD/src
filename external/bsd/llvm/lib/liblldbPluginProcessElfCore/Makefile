#	$NetBSD: Makefile,v 1.1 2013/09/29 12:12:01 joerg Exp $

LIB=	lldbPluginProcessElfCore

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/elf-core

SRCS+=	ProcessElfCore.cpp \
	ThreadElfCore.cpp \
	RegisterContextPOSIXCore_x86_64.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
