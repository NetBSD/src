#	$NetBSD: Makefile,v 1.1.8.1 2012/11/20 02:58:00 tls Exp $

PROG_CXX=	clang-tblgen
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/utils/TableGen

SRCS=	ClangASTNodesEmitter.cpp \
	ClangAttrEmitter.cpp \
	ClangCommentCommandInfoEmitter.cpp \
	ClangCommentHTMLTagsEmitter.cpp \
	ClangDiagnosticsEmitter.cpp \
	ClangSACheckersEmitter.cpp \
	NeonEmitter.cpp \
	OptParserEmitter.cpp \
	TableGen.cpp

LLVM_LIBS+= \
	TableGen \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
