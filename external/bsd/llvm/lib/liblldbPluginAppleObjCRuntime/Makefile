#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:28 tls Exp $

LIB=	lldbPluginAppleObjCRuntime

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/LanguageRuntime/ObjC/AppleObjCRuntime

SRCS+=	AppleObjCRuntime.cpp \
	AppleObjCRuntimeV1.cpp \
	AppleObjCRuntimeV2.cpp \
	AppleObjCTrampolineHandler.cpp \
	AppleObjCTypeVendor.cpp \
	AppleThreadPlanStepThroughObjCTrampoline.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
