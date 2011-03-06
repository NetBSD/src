#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:52 bouyer Exp $

LIB=	clangDriver

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Driver

SRCS+=	Action.cpp \
	Arg.cpp \
	ArgList.cpp \
	CC1Options.cpp \
	CC1AsOptions.cpp \
	Compilation.cpp \
	Driver.cpp \
	DriverOptions.cpp \
	HostInfo.cpp \
	Job.cpp \
	Option.cpp \
	OptTable.cpp \
	Phases.cpp \
	Tool.cpp \
	ToolChain.cpp \
	ToolChains.cpp \
	Tools.cpp \
	Types.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
