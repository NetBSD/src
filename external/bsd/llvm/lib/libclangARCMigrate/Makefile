#	$NetBSD: Makefile,v 1.3 2011/11/18 15:56:54 joerg Exp $

LIB=	clangARCMigrate

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/ARCMigrate

SRCS+=	ARCMT.cpp \
	ARCMTActions.cpp \
	FileRemapper.cpp \
	PlistReporter.cpp \
	TransAPIUses.cpp \
	TransARCAssign.cpp \
	TransAutoreleasePool.cpp \
	TransBlockObjCVariable.cpp \
	TransEmptyStatementsAndDealloc.cpp \
	TransGCAttrs.cpp \
	TransGCCalls.cpp \
	TransProperties.cpp \
	TransRetainReleaseDealloc.cpp \
	TransUnbridgedCasts.cpp \
	TransUnusedInitDelegate.cpp \
	TransZeroOutPropsInDealloc.cpp \
	Transforms.cpp \
	TransformActions.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
