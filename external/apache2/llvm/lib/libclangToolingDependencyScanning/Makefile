#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:58 joerg Exp $

LIB=	clangToolingDependencyScanning

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/DependencyScanning/

SRCS+=	DependencyScanningFilesystem.cpp \
	DependencyScanningService.cpp \
	DependencyScanningTool.cpp \
	DependencyScanningWorker.cpp \
	ModuleDepCollector.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
