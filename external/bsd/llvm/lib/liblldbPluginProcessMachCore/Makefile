#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 16:14:50 yamt Exp $

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
