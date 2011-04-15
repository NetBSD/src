#	$NetBSD: Makefile,v 1.2 2011/04/15 19:31:07 joerg Exp $

.include <bsd.init.mk>

.PATH:	${CLANG_SRCDIR}/lib/Headers

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS=	avxintrin.h \
	emmintrin.h \
	immintrin.h \
	mm_malloc.h \
	mmintrin.h \
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

TABLEGEN_SRC=	Intrinsics.td Attr.td StmtNodes.td DeclNodes.td \
	Diagnostic.td arm_neon.td Options.td CC1AsOptions.td CC1Options.td

TABLEGEN_OUTPUT.Intrinsics.td= \
	llvm/Intrinsics.gen|-gen-intrinsic

TABLEGEN_INCLUDES.Attr.td=	-I${CLANG_SRCDIR}/include
TABLEGEN_OUTPUT.Attr.td= \
	clang/AST/Attrs.inc|-gen-clang-attr-classes \
        clang/AST/AttrImpl.inc|-gen-clang-attr-impl \
	clang/Basic/AttrList.inc|-gen-clang-attr-list \
	clang/Lex/AttrSpellings.inc|-gen-clang-attr-spelling-list \
	clang/Serialization/AttrPCHRead.inc|-gen-clang-attr-pch-read \
	clang/Serialization/AttrPCHWrite.inc|-gen-clang-attr-pch-write

TABLEGEN_OUTPUT.StmtNodes.td= \
	clang/AST/StmtNodes.inc|-gen-clang-stmt-nodes

TABLEGEN_OUTPUT.DeclNodes.td= \
	clang/AST/DeclNodes.inc|-gen-clang-decl-nodes

TABLEGEN_INCLUDES.Diagnostic.td=	-I${CLANG_SRCDIR}/include/clang/Basic
TABLEGEN_OUTPUT.Diagnostic.td= \
	clang/Basic/DiagnosticAnalysisKinds.inc|-gen-clang-diags-defs^-clang-component=Analysis \
	clang/Basic/DiagnosticASTKinds.inc|-gen-clang-diags-defs^-clang-component=AST \
	clang/Basic/DiagnosticCommonKinds.inc|-gen-clang-diags-defs^-clang-component=Common \
	clang/Basic/DiagnosticDriverKinds.inc|-gen-clang-diags-defs^-clang-component=Driver \
	clang/Basic/DiagnosticFrontendKinds.inc|-gen-clang-diags-defs^-clang-component=Frontend \
	clang/Basic/DiagnosticLexKinds.inc|-gen-clang-diags-defs^-clang-component=Lex \
	clang/Basic/DiagnosticParseKinds.inc|-gen-clang-diags-defs^-clang-component=Parse \
	clang/Basic/DiagnosticSemaKinds.inc|-gen-clang-diags-defs^-clang-component=Sema \
	clang/Basic/DiagnosticGroups.inc|-gen-clang-diag-groups

TABLEGEN_OUTPUT.arm_neon.td= \
	clang/Basic/arm_neon.inc|-gen-arm-neon-sema \
	arm_neon.h.inc|-gen-arm-neon

TABLEGEN_INCLUDES.Options.td=	-I${CLANG_SRCDIR}/include/clang/Driver
TABLEGEN_OUTPUT.Options.td= \
	clang/Driver/Options.inc|-gen-opt-parser-defs

TABLEGEN_INCLUDES.CC1Options.td=	-I${CLANG_SRCDIR}/include/clang/Driver
TABLEGEN_OUTPUT.CC1Options.td= \
	clang/Driver/CC1Options.inc|-gen-opt-parser-defs

TABLEGEN_INCLUDES.CC1AsOptions.td=	-I${CLANG_SRCDIR}/include/clang/Driver
TABLEGEN_OUTPUT.CC1AsOptions.td= \
	clang/Driver/CC1AsOptions.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../tablegen.mk"

depend dependall all: ${DPSRCS}

.include <bsd.prog.mk>
