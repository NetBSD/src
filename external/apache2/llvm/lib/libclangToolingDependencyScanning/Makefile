#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:13 cjep Exp $

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
