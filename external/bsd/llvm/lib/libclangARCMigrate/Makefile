#	$NetBSD: Makefile,v 1.2 2011/07/25 16:32:44 joerg Exp $

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
