#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 16:14:50 yamt Exp $

LIB=	lldbPluginProcessPOSIX

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/POSIX

CPPFLAGS.POSIXThread.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.ProcessPOSIX.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIX_x86.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIXProcessMonitor_x86.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.RegisterContextPOSIXProcessMonitor_mips64.cpp= -I${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD
CPPFLAGS.POSIXThread.cpp+=	-I${LLDB_SRCDIR}/source/Plugins/Process/Utility

SRCS+=	POSIXStopInfo.cpp \
	POSIXThread.cpp \
	ProcessMessage.cpp \
	ProcessPOSIX.cpp \
	ProcessPOSIXLog.cpp \
	RegisterContextPOSIX_mips64.cpp \
	RegisterContextPOSIX_x86.cpp \
	RegisterContextFreeBSD_i386.cpp \
	RegisterContextFreeBSD_mips64.cpp \
	RegisterContextFreeBSD_x86_64.cpp \
	RegisterContextLinux_i386.cpp \
	RegisterContextLinux_x86_64.cpp \
	RegisterContextPOSIXProcessMonitor_mips64.cpp \
	RegisterContextPOSIXProcessMonitor_x86.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
