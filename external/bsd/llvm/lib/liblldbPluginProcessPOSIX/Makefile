#	$NetBSD: Makefile,v 1.1 2013/09/29 12:12:02 joerg Exp $

LIB=	lldbPluginProcessPOSIX

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/POSIX

CPPFLAGS.POSIXThread.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.ProcessPOSIX.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIX_i386.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIX_x86_64.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIXProcessMonitor_i386.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIXProcessMonitor_x86_64.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD

SRCS+=	POSIXStopInfo.cpp \
	POSIXThread.cpp \
	ProcessMessage.cpp \
	ProcessPOSIX.cpp \
	ProcessPOSIXLog.cpp \
	RegisterContextPOSIX_i386.cpp \
	RegisterContextPOSIX_x86_64.cpp \
	RegisterContextFreeBSD_x86_64.cpp \
	RegisterContextLinux_x86_64.cpp \
	RegisterContextPOSIXProcessMonitor_i386.cpp \
	RegisterContextPOSIXProcessMonitor_x86_64.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
