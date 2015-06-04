#	$NetBSD: Makefile,v 1.1.6.1 2015/06/04 20:05:47 snj Exp $

LIB=	lldbPluginAppleObjCRuntime

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/LanguageRuntime/ObjC/AppleObjCRuntime

SRCS+=	AppleObjCDeclVendor.cpp \
	AppleObjCRuntime.cpp \
	AppleObjCRuntimeV1.cpp \
	AppleObjCRuntimeV2.cpp \
	AppleObjCTrampolineHandler.cpp \
	AppleThreadPlanStepThroughObjCTrampoline.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
