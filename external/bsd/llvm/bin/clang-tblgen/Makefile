#	$NetBSD: Makefile,v 1.4.10.1 2017/03/20 06:52:27 pgoyette Exp $

PROG_CXX=	clang-tblgen
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/utils/TableGen

SRCS=	ClangASTNodesEmitter.cpp \
	ClangAttrEmitter.cpp \
	ClangCommentCommandInfoEmitter.cpp \
	ClangCommentHTMLNamedCharacterReferenceEmitter.cpp \
	ClangCommentHTMLTagsEmitter.cpp \
	ClangDiagnosticsEmitter.cpp \
	ClangSACheckersEmitter.cpp \
	NeonEmitter.cpp \
	TableGen.cpp

LLVM_LIBS+= \
	TableGen \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
