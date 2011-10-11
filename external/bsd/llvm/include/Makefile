#	$NetBSD: Makefile,v 1.4 2011/10/11 13:53:58 joerg Exp $

.include <bsd.init.mk>

.PATH:	${CLANG_SRCDIR}/lib/Headers

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS=	avxintrin.h \
	emmintrin.h \
	immintrin.h \
	mm3dnow.h \
	mmintrin.h \
	mm_malloc.h \
	pmmintrin.h \
	smmintrin.h \
	tmmintrin.h \
	xmmintrin.h
.elif ${MACHINE_ARCH} == "powerpc"
INCS=	altivec.h
.endif
INCSDIR=	/usr/include/clang-3.0

HEADER=	\
  float.h \
  iso646.h \
  limits.h \
  stdarg.h \
  stdbool.h \
  stddef.h \
  stdint.h \
  tgmath.h

.PATH:	${LLVM_SRCDIR}/include/llvm \
	${CLANG_SRCDIR}/include/clang/Basic \
	${CLANG_SRCDIR}/include/clang/Driver

TABLEGEN_SRC=	Intrinsics.td

TABLEGEN_OUTPUT.Intrinsics.td= \
	llvm/Intrinsics.gen|-gen-intrinsic

CLANG_TABLEGEN_SRC=	Attr.td StmtNodes.td DeclNodes.td \
	Diagnostic.td arm_neon.td Options.td CC1AsOptions.td CC1Options.td

CLANG_TABLEGEN_INCLUDES.Attr.td=	-I${CLANG_SRCDIR}/include
CLANG_TABLEGEN_OUTPUT.Attr.td= \
	clang/AST/Attrs.inc|-gen-clang-attr-classes \
        clang/AST/AttrImpl.inc|-gen-clang-attr-impl \
	clang/Basic/AttrList.inc|-gen-clang-attr-list \
	clang/Lex/AttrSpellings.inc|-gen-clang-attr-spelling-list \
	clang/Parse/AttrLateParsed.inc|-gen-clang-attr-late-parsed-list \
	clang/Serialization/AttrPCHRead.inc|-gen-clang-attr-pch-read \
	clang/Serialization/AttrPCHWrite.inc|-gen-clang-attr-pch-write

CLANG_TABLEGEN_OUTPUT.StmtNodes.td= \
	clang/AST/StmtNodes.inc|-gen-clang-stmt-nodes

CLANG_TABLEGEN_OUTPUT.DeclNodes.td= \
	clang/AST/DeclNodes.inc|-gen-clang-decl-nodes

CLANG_TABLEGEN_INCLUDES.Diagnostic.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.Diagnostic.td= \
	clang/Basic/DiagnosticAnalysisKinds.inc|-gen-clang-diags-defs^-clang-component=Analysis \
	clang/Basic/DiagnosticASTKinds.inc|-gen-clang-diags-defs^-clang-component=AST \
	clang/Basic/DiagnosticCommonKinds.inc|-gen-clang-diags-defs^-clang-component=Common \
	clang/Basic/DiagnosticDriverKinds.inc|-gen-clang-diags-defs^-clang-component=Driver \
	clang/Basic/DiagnosticFrontendKinds.inc|-gen-clang-diags-defs^-clang-component=Frontend \
	clang/Basic/DiagnosticLexKinds.inc|-gen-clang-diags-defs^-clang-component=Lex \
	clang/Basic/DiagnosticParseKinds.inc|-gen-clang-diags-defs^-clang-component=Parse \
	clang/Basic/DiagnosticSemaKinds.inc|-gen-clang-diags-defs^-clang-component=Sema \
	clang/Basic/DiagnosticGroups.inc|-gen-clang-diag-groups \
	clang/Basic/DiagnosticIndexName.inc|-gen-clang-diags-index-name

CLANG_TABLEGEN_OUTPUT.arm_neon.td= \
	clang/Basic/arm_neon.inc|-gen-arm-neon-sema \
	arm_neon.h.inc|-gen-arm-neon

CLANG_TABLEGEN_INCLUDES.Options.td=	-I${CLANG_SRCDIR}/include/clang/Driver
CLANG_TABLEGEN_OUTPUT.Options.td= \
	clang/Driver/Options.inc|-gen-opt-parser-defs

CLANG_TABLEGEN_INCLUDES.CC1Options.td=	-I${CLANG_SRCDIR}/include/clang/Driver
CLANG_TABLEGEN_OUTPUT.CC1Options.td= \
	clang/Driver/CC1Options.inc|-gen-opt-parser-defs

CLANG_TABLEGEN_INCLUDES.CC1AsOptions.td=	-I${CLANG_SRCDIR}/include/clang/Driver
CLANG_TABLEGEN_OUTPUT.CC1AsOptions.td= \
	clang/Driver/CC1AsOptions.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../tablegen.mk"

depend dependall all: ${DPSRCS}

.include <bsd.prog.mk>
