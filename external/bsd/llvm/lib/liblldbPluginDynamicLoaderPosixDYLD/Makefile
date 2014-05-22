#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:48 yamt Exp $

LIB=	lldbPluginDynamicLoaderPosixDYLD

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/DynamicLoader/POSIX-DYLD

SRCS+=	AuxVector.cpp \
	DYLDRendezvous.cpp \
	DynamicLoaderPOSIXDYLD.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
