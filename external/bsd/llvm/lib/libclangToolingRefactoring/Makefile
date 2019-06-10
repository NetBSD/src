#	$NetBSD: Makefile,v 1.1.4.1 2019/06/10 21:50:58 christos Exp $

LIB=	clangToolingRefactoring

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Refactoring

SRCS+=	ASTSelection.cpp \
	ASTSelectionRequirements.cpp \
	AtomicChange.cpp \
	RefactoringActions.cpp

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Refactoring/Extract

SRCS+=	Extract.cpp \
	SourceExtraction.cpp

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Refactoring/Rename

SRCS+=	RenamingAction.cpp \
	SymbolOccurrences.cpp \
	USRFinder.cpp \
	USRFindingAction.cpp \
	USRLocFinder.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
