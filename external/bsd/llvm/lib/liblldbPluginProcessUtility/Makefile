#	$NetBSD: Makefile,v 1.2.2.1 2014/08/10 07:09:11 tls Exp $

LIB=	lldbPluginProcessUtility

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/Utility

SRCS+=	DynamicRegisterInfo.cpp \
	FreeBSDSignals.cpp \
	HistoryThread.cpp \
	HistoryUnwind.cpp \
	InferiorCallPOSIX.cpp \
	RegisterContextDarwin_arm.cpp \
	RegisterContextDarwin_arm64.cpp \
	RegisterContextDarwin_i386.cpp \
	RegisterContextDarwin_x86_64.cpp \
	RegisterContextDummy.cpp \
	RegisterContextFreeBSD_i386.cpp \
	RegisterContextFreeBSD_mips64.cpp \
	RegisterContextFreeBSD_x86_64.cpp \
	RegisterContextLinux_i386.cpp \
	RegisterContextLinux_x86_64.cpp \
	RegisterContextHistory.cpp \
	RegisterContextLLDB.cpp \
	RegisterContextMach_arm.cpp \
	RegisterContextMach_i386.cpp \
	RegisterContextMach_x86_64.cpp \
	RegisterContextMacOSXFrameBackchain.cpp \
	RegisterContextMemory.cpp \
	RegisterContextPOSIX_mips64.cpp \
	RegisterContextPOSIX_x86.cpp \
	RegisterContextThreadMemory.cpp \
	StopInfoMachException.cpp \
	ThreadMemory.cpp \
	UnwindLLDB.cpp \
	UnwindMacOSXFrameBackchain.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
