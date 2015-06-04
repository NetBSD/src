#	$NetBSD: Makefile,v 1.3.2.1 2015/06/04 20:05:47 snj Exp $

LIB=	lldbPluginProcessPOSIX

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/POSIX

CPPFLAGS.POSIXStopInfo.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.POSIXThread.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.ProcessPOSIX.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIX_x86.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.RegisterContextPOSIXProcessMonitor_mips64.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIXProcessMonitor_powerpc.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIXProcessMonitor_x86.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/Utility -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.POSIXThread.cpp+=	-I${LLDB_SRCDIR}/source/Plugins/Process/Utility

SRCS+=	POSIXStopInfo.cpp \
	POSIXThread.cpp \
	ProcessMessage.cpp \
	ProcessPOSIX.cpp \
	ProcessPOSIXLog.cpp \
	RegisterContextPOSIXProcessMonitor_mips64.cpp \
	RegisterContextPOSIXProcessMonitor_powerpc.cpp \
	RegisterContextPOSIXProcessMonitor_x86.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
