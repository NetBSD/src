#	$NetBSD: Makefile,v 1.3.2.1 2015/06/04 20:05:47 snj Exp $

LIB=	lldbPluginProcessElfCore

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/elf-core

CPPFLAGS.RegisterContextPOSIXCore_mips64.cpp+= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.RegisterContextPOSIXCore_powerpc.cpp+= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.RegisterContextPOSIXCore_x86_64.cpp+= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.ThreadElfCore.cpp+= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility

SRCS+=	ProcessElfCore.cpp \
	RegisterContextPOSIXCore_mips64.cpp \
	RegisterContextPOSIXCore_powerpc.cpp \
	RegisterContextPOSIXCore_x86_64.cpp \
	ThreadElfCore.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
