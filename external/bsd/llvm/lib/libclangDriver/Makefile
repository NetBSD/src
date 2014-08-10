#	$NetBSD: Makefile,v 1.8 2014/08/10 17:34:22 joerg Exp $

LIB=	clangDriver

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Driver

SRCS+=	Action.cpp \
	Compilation.cpp \
	Driver.cpp \
	DriverOptions.cpp \
	Job.cpp \
	Multilib.cpp \
	Phases.cpp \
	SanitizerArgs.cpp \
	Tool.cpp \
	ToolChain.cpp \
	ToolChains.cpp \
	Tools.cpp \
	Types.cpp \
	WindowsToolChain.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
