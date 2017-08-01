#	$NetBSD: Makefile,v 1.1 2017/08/01 20:07:43 joerg Exp $

LIB=	clangToolingRefactoring

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Refactoring

SRCS+=	AtomicChange.cpp

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Refactoring/Rename

SRCS+=	RenamingAction.cpp \
	USRFinder.cpp \
	USRFindingAction.cpp \
	USRLocFinder.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
