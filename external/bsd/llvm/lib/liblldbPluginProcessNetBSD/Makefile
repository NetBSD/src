#	$NetBSD: Makefile,v 1.2.4.2 2014/08/19 23:51:30 tls Exp $

LIB=	lldbPluginProcessNetBSD

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/FreeBSD

CPPFLAGS.ProcessFreeBSD.cpp+=	-I${LLDB_SRCDIR}/source/Plugins/Process/Utility
CPPFLAGS.ProcessMonitor.cpp+=	-I${LLDB_SRCDIR}/source/Plugins/Process/Utility

SRCS+=	ProcessFreeBSD.cpp \
	ProcessMonitor.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
