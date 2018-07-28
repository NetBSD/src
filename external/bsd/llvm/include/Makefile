#	$NetBSD: Makefile,v 1.41.2.2 2018/07/28 04:37:09 pgoyette Exp $

.include <bsd.init.mk>

.PATH:	${CLANG_SRCDIR}/lib/Headers


.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS=	adxintrin.h \
	ammintrin.h \
	avx2intrin.h \
	avx512bitalgintrin.h \
	avx512bwintrin.h \
	avx512cdintrin.h \
	avx512dqintrin.h \
	avx512erintrin.h \
	avx512fintrin.h \
	avx512ifmaintrin.h \
	avx512ifmavlintrin.h \
	avx512pfintrin.h \
	avx512vbmi2intrin.h \
	avx512vbmiintrin.h \
	avx512vbmivlintrin.h \
	avx512vlbitalgintrin.h \
	avx512vlbwintrin.h \
	avx512vlcdintrin.h \
	avx512vldqintrin.h \
	avx512vlintrin.h \
	avx512vlvbmi2intrin.h \
	avx512vlvnniintrin.h \
	avx512vnniintrin.h \
	avx512vpopcntdqintrin.h \
	avx512vpopcntdqvlintrin.h \
	avxintrin.h \
	bmi2intrin.h \
	bmiintrin.h \
	cetintrin.h \
	cldemoteintrin.h \
	clflushoptintrin.h \
	clwbintrin.h \
	clzerointrin.h \
	cpuid.h \
	emmintrin.h \
	f16cintrin.h \
	fma4intrin.h \
	fmaintrin.h \
	fxsrintrin.h \
	gfniintrin.h \
	ia32intrin.h \
	immintrin.h \
	invpcidintrin.h \
	lwpintrin.h \
	lzcntintrin.h \
	mm3dnow.h \
	mmintrin.h \
	mm_malloc.h \
	movdirintrin.h \
	mwaitxintrin.h \
	nmmintrin.h \
	pconfigintrin.h \
	pkuintrin.h \
	pmmintrin.h \
	popcntintrin.h \
	prfchwintrin.h \
	ptwriteintrin.h \
	rdseedintrin.h \
	rtmintrin.h \
	sgxintrin.h \
	shaintrin.h \
	smmintrin.h \
	tbmintrin.h \
	tmmintrin.h \
	vaesintrin.h \
	vpclmulqdqintrin.h \
	waitpkgintrin.h \
	wbnoinvdintrin.h \
	wmmintrin.h \
	__wmmintrin_aes.h \
	__wmmintrin_pclmul.h \
	x86intrin.h \
	xmmintrin.h \
	xopintrin.h \
	xsavecintrin.h \
	xsaveintrin.h \
	xsaveoptintrin.h \
	xsavesintrin.h \
	xtestintrin.h

.elif ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpc64"
INCS=	altivec.h \
	htmintrin.h \
	htmxlintrin.h
.elif ${MACHINE_CPU} == "arm" || ${MACHINE_CPU} == "aarch64"
INCS=	arm_acle.h \
	arm_neon.h
.  if ${MACHINE_CPU} == "aarch64"
INCS+=	arm64intr.h
.  endif
.elif ${MACHINE_CPU} == "mips" || ${MACHINE_CPU} == "mips64"
INCS=	msa.h
.endif
INCS+=	stdatomic.h
INCSDIR=	/usr/include/clang-${CLANG_VERSION:R}

.PATH:	${LLVM_SRCDIR}/include/llvm/IR \
	${CLANG_SRCDIR}/include/clang/AST \
	${CLANG_SRCDIR}/include/clang/Basic \
	${CLANG_SRCDIR}/include/clang/Driver \
	${CLANG_SRCDIR}/include/clang/StaticAnalyzer/Checkers

TABLEGEN_SRC=	Attributes.td Intrinsics.td Options.td

TABLEGEN_OUTPUT.Attributes.td= \
	llvm/IR/Attributes.inc|-gen-attrs

TABLEGEN_OUTPUT.Intrinsics.td= \
	llvm/IR/IntrinsicEnums.inc|-gen-intrinsic-enums \
	llvm/IR/IntrinsicImpl.inc|-gen-intrinsic-impl

CLANG_TABLEGEN_SRC=	\
	arm_fp16.td \
	arm_neon.td \
	Attr.td \
	Checkers.td \
	CommentCommands.td \
	CommentHTMLNamedCharacterReferences.td \
	CommentHTMLTags.td \
	CommentNodes.td \
	DeclNodes.td \
	Diagnostic.td \
	StmtDataCollectors.td \
	StmtNodes.td

CLANG_TABLEGEN_INCLUDES.Attr.td=	-I${CLANG_SRCDIR}/include
CLANG_TABLEGEN_OUTPUT.Attr.td= \
	clang/AST/Attrs.inc|-gen-clang-attr-classes \
	clang/AST/AttrDump.inc|-gen-clang-attr-dump \
	clang/AST/AttrImpl.inc|-gen-clang-attr-impl \
	clang/AST/AttrVisitor.inc|-gen-clang-attr-ast-visitor \
	clang/Basic/AttrList.inc|-gen-clang-attr-list \
	clang/Basic/AttrHasAttributeImpl.inc|-gen-clang-attr-has-attribute-impl \
	clang/Basic/AttrSubMatchRulesList.inc|-gen-clang-attr-subject-match-rule-list \
	clang/Parse/AttrParserStringSwitches.inc|-gen-clang-attr-parser-string-switches \
	clang/Parse/AttrSubMatchRulesParserStringSwitches.inc|-gen-clang-attr-subject-match-rules-parser-string-switches \
	clang/Sema/AttrParsedAttrImpl.inc|-gen-clang-attr-parsed-attr-impl \
	clang/Sema/AttrParsedAttrKinds.inc|-gen-clang-attr-parsed-attr-kinds \
	clang/Sema/AttrParsedAttrList.inc|-gen-clang-attr-parsed-attr-list \
	clang/Sema/AttrSpellingListIndex.inc|-gen-clang-attr-spelling-index \
	clang/Sema/AttrTemplateInstantiate.inc|-gen-clang-attr-template-instantiate \
	clang/Serialization/AttrPCHRead.inc|-gen-clang-attr-pch-read \
	clang/Serialization/AttrPCHWrite.inc|-gen-clang-attr-pch-write

CLANG_TABLEGEN_OUTPUT.StmtNodes.td= \
	clang/AST/StmtNodes.inc|-gen-clang-stmt-nodes

CLANG_TABLEGEN_OUTPUT.StmtDataCollectors.td= \
	clang/AST/StmtDataCollectors.inc|-gen-clang-data-collectors

CLANG_TABLEGEN_OUTPUT.DeclNodes.td= \
	clang/AST/DeclNodes.inc|-gen-clang-decl-nodes

CLANG_TABLEGEN_OUTPUT.CommentNodes.td= \
	clang/AST/CommentNodes.inc|-gen-clang-comment-nodes

CLANG_TABLEGEN_OUTPUT.CommentCommands.td= \
	clang/AST/CommentCommandInfo.inc|-gen-clang-comment-command-info \
	clang/AST/CommentCommandList.inc|-gen-clang-comment-command-list

CLANG_TABLEGEN_OUTPUT.CommentHTMLTags.td= \
	clang/AST/CommentHTMLTags.inc|-gen-clang-comment-html-tags \
	clang/AST/CommentHTMLTagsProperties.inc|-gen-clang-comment-html-tags-properties

CLANG_TABLEGEN_OUTPUT.CommentHTMLNamedCharacterReferences.td= \
	clang/AST/CommentHTMLNamedCharacterReferences.inc|-gen-clang-comment-html-named-character-references

CLANG_TABLEGEN_INCLUDES.Diagnostic.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.Diagnostic.td= \
	clang/Basic/DiagnosticAnalysisKinds.inc|-gen-clang-diags-defs^-clang-component=Analysis \
	clang/Basic/DiagnosticASTKinds.inc|-gen-clang-diags-defs^-clang-component=AST \
	clang/Basic/DiagnosticCommentKinds.inc|-gen-clang-diags-defs^-clang-component=Comment \
	clang/Basic/DiagnosticCommonKinds.inc|-gen-clang-diags-defs^-clang-component=Common \
	clang/Basic/DiagnosticCrossTUKinds.inc|-gen-clang-diags-defs^-clang-component=CrossTU \
	clang/Basic/DiagnosticDriverKinds.inc|-gen-clang-diags-defs^-clang-component=Driver \
	clang/Basic/DiagnosticFrontendKinds.inc|-gen-clang-diags-defs^-clang-component=Frontend \
	clang/Basic/DiagnosticGroups.inc|-gen-clang-diag-groups \
	clang/Basic/DiagnosticIndexName.inc|-gen-clang-diags-index-name \
	clang/Basic/DiagnosticLexKinds.inc|-gen-clang-diags-defs^-clang-component=Lex \
	clang/Basic/DiagnosticParseKinds.inc|-gen-clang-diags-defs^-clang-component=Parse \
	clang/Basic/DiagnosticRefactoringKinds.inc|-gen-clang-diags-defs^-clang-component=Refactoring \
	clang/Basic/DiagnosticSemaKinds.inc|-gen-clang-diags-defs^-clang-component=Sema \
	clang/Basic/DiagnosticSerializationKinds.inc|-gen-clang-diags-defs^-clang-component=Serialization


CLANG_TABLEGEN_INCLUDES.arm_neon.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.arm_neon.td= \
	clang/Basic/arm_neon.inc|-gen-arm-neon-sema \
	arm_neon.h|-gen-arm-neon

CLANG_TABLEGEN_INCLUDES.arm_fp16.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.arm_fp16.td= \
	clang/Basic/arm_fp16.inc|-gen-arm-neon-sema

CLANG_TABLEGEN_INCLUDES.Checkers.td=	\
	-I${CLANG_SRCDIR}/include \
	-I${CLANG_SRCDIR}/include/clang/StaticAnalyzer/Checkers
CLANG_TABLEGEN_OUTPUT.Checkers.td= \
	clang/StaticAnalyzer/Checkers/Checkers.inc|-gen-clang-sa-checkers

TABLEGEN_INCLUDES.Options.td=	-I${CLANG_SRCDIR}/include/clang/Driver
TABLEGEN_OUTPUT.Options.td= \
	clang/Driver/Options.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../tablegen.mk"

DPSRCS+=	clang/Basic/Version.inc llvm/Support/VCSRevision.h
CLEANFILES+=	clang/Basic/Version.inc llvm/Support/VCSRevision.h

clang/Basic/Version.inc: ${LLVM_TOPLEVEL}/Makefile.inc
	mkdir -p ${.TARGET:H}
	printf '#define CLANG_VERSION ${CLANG_VERSION}\n' > ${.TARGET}.tmp
	printf '#define CLANG_VERSION_STRING "${CLANG_VERSION}"\n' >> ${.TARGET}.tmp
	printf '#define CLANG_VERSION_MAJOR ${CLANG_VERSION:R:R}\n' >> ${.TARGET}.tmp
	printf '#define CLANG_VERSION_MINOR ${CLANG_VERSION:R:E}\n' >> ${.TARGET}.tmp
	printf '#define CLANG_VERSION_PATCHLEVEL ${CLANG_VERSION:R:R}\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

llvm/Support/VCSRevision.h: ${LLVM_TOPLEVEL}/Makefile.inc
	mkdir -p ${.TARGET:H}
	printf '#define LLVM_DEFAULT_TARGET_TRIPLE "${MACHINE_GNU_PLATFORM}"\n' >> ${.TARGET}.tmp
	printf '#define LLVM_REVISION "svn-r${LLVM_REVISION}"\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.if ${MKLLD} != "no"
DPSRCS+=	lld/Config/Version.inc
CLEANFILES+=	lld/Config/Version.inc

lld/Config/Version.inc: ${LLVM_TOPLEVEL}/Makefile.inc
	mkdir -p ${.TARGET:H}
	printf '#define LLD_VERSION ${LLD_VERSION}\n' > ${.TARGET}.tmp
	printf '#define LLD_VERSION_MAJOR ${LLD_VERSION:R:R}\n' >> ${.TARGET}.tmp
	printf '#define LLD_VERSION_MINOR ${LLD_VERSION:R:E}\n' >> ${.TARGET}.tmp
	printf '#define LLD_REVISION_STRING "r${LLD_REVISION}"\n' >> ${.TARGET}.tmp
	printf '#define LLD_REPOSITORY_STRING "${SVN_ROOT}/lld/${SVN_BRANCH}"\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}
.endif

.ifndef HOST_BUILD

DPSRCS+=	llvm/Config/config.h llvm/Config/llvm-config.h
CLEANFILES+=	llvm/Config/config.h llvm/Config/llvm-config.h

NATIVE_LIBS=	-lm -lexecinfo

llvm/Config/config.h: ${LLVM_TOPLEVEL}/Makefile.inc ${LLVM_TOPLEVEL}/config/llvm/Config/config.h.in
	mkdir -p ${.TARGET:H}
	${TOOL_SED} -e /LLVM_DEFAULT_TARGET_TRIPLE/d -e /LLVM_NATIVE_/d \
	    -e /LLVM_VERSION_/d -e /PACKAGE_VERSION/d -e 'x' \
	    < ${LLVM_TOPLEVEL}/config/llvm/Config/config.h.in \
	    > ${.TARGET}.tmp
	printf '#define LLVM_DEFAULT_TARGET_TRIPLE "${MACHINE_GNU_PLATFORM}"\n' >> ${.TARGET}.tmp
	printf '#define PACKAGE_VERSION "${LLVM_VERSION} (trunk ${LLVM_REVISION})"\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_MAJOR ${LLVM_VERSION:R:R}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_MINOR ${LLVM_VERSION:R:E}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_PATCH ${LLVM_VERSION:E}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_STRING "${LLVM_VERSION}"\n' >> ${.TARGET}.tmp
	printf '#endif\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

llvm/Config/llvm-config.h: ${LLVM_TOPLEVEL}/Makefile.inc ${LLVM_TOPLEVEL}/config/llvm/Config/llvm-config.h.in
	mkdir -p ${.TARGET:H}
	${TOOL_SED} -e /LLVM_DEFAULT_TARGET_TRIPLE/d -e /LLVM_NATIVE_/d \
	    -e /LLVM_VERSION_/d -e /PACKAGE_VERSION/d -e 'x' \
	    < ${LLVM_TOPLEVEL}/config/llvm/Config/llvm-config.h.in \
	    > ${.TARGET}.tmp
	printf '#define LLVM_DEFAULT_TARGET_TRIPLE "${MACHINE_GNU_PLATFORM}"\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_MAJOR ${LLVM_VERSION:R:R}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_MINOR ${LLVM_VERSION:R:E}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_PATCH ${LLVM_VERSION:E}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_STRING "${LLVM_VERSION}"\n' >> ${.TARGET}.tmp
	printf '#endif\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

run-configure:
	[ ! -d tmp ] || rm -r tmp
	mkdir -p tmp
	cd tmp && CONFIG_SITE= ${CONFIG_DIR}/configure ${LLVM_CONFIGURE_ARGS} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:N-Werror:Q} CXX=${CXX:Q} CXXFLAGS=${CXXFLAGS:N-Werror:Q} \
	    CPPFLAGS=${CPPFLAGS:N-std=c++11:Q} LDFLAGS=${LDFLAGS:Q} LD=${LD:Q} LIBS=${NATIVE_LIBS:Q} \
	    BUILD_CC=${CC:Q} BUILD_CXX=${CXX:Q}
	cp -r tmp/include/llvm/* ${LLVM_INCLUDE_CONFIG}/llvm/
	mv ${LLVM_INCLUDE_CONFIG}/llvm/Config/llvm-config.h \
	   ${LLVM_INCLUDE_CONFIG}/llvm/Config/llvm-config.h.in
	cp -r tmp/include/clang/* ${CLANG_INCLUDE_CONFIG}/clang/
	rm -r tmp
	mv ${LLVM_INCLUDE_CONFIG}/llvm/Config/config.h \
	    ${LLVM_INCLUDE_CONFIG}/llvm/Config/config.h.in
.endif

depend dependall all: ${DPSRCS}

.include <bsd.prog.mk>
