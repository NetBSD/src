#	$NetBSD: Makefile,v 1.2.8.2 2014/08/19 23:51:30 tls Exp $

LIB=	lldbPluginProcessMachCore

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Process/mach-core

CPPFLAGS.ProcessMachCore.cpp+=	-I${LLDB_SRCDIR}/source/Plugins/Process/Utility

SRCS+=	ProcessMachCore.cpp \
	ThreadMachCore.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
