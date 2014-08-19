#	$NetBSD: Makefile,v 1.3.4.2 2014/08/19 23:51:30 tls Exp $

LIB=	lldbPluginProcessElfCore

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/elf-core

CPPFLAGS.RegisterContextPOSIXCore_mips64.cpp+= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.RegisterContextPOSIXCore_x86_64.cpp+= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.ThreadElfCore.cpp+= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility

SRCS+=	ProcessElfCore.cpp \
	RegisterContextPOSIXCore_mips64.cpp \
	RegisterContextPOSIXCore_x86_64.cpp \
	ThreadElfCore.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
