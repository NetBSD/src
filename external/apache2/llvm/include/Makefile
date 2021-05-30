#	$NetBSD: Makefile,v 1.3 2021/05/30 01:56:50 joerg Exp $

.include <bsd.init.mk>

.if ${MKLLVM} != "no"
.PATH:	${CLANG_SRCDIR}/lib/Headers


.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS=	adxintrin.h \
	ammintrin.h \
	amxintrin.h \
	avx2intrin.h \
	avx512bf16intrin.h \
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
	avx512vlbf16intrin.h \
	avx512vlbitalgintrin.h \
	avx512vlbwintrin.h \
	avx512vlcdintrin.h \
	avx512vldqintrin.h \
	avx512vlintrin.h \
	avx512vlvbmi2intrin.h \
	avx512vlvnniintrin.h \
	avx512vlvp2intersectintrin.h \
	avx512vnniintrin.h \
	avx512vp2intersectintrin.h \
	avx512vpopcntdqintrin.h \
	avx512vpopcntdqvlintrin.h \
	avxintrin.h \
	avxvnniintrin.h \
	bmi2intrin.h \
	bmiintrin.h \
	cetintrin.h \
	cldemoteintrin.h \
	clflushoptintrin.h \
	clwbintrin.h \
	clzerointrin.h \
	cpuid.h \
	emmintrin.h \
	enqcmdintrin.h \
	f16cintrin.h \
	fma4intrin.h \
	fmaintrin.h \
	fxsrintrin.h \
	gfniintrin.h \
	hresetintrin.h \
	ia32intrin.h \
	immintrin.h \
	invpcidintrin.h \
	keylockerintrin.h \
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
	serializeintrin.h \
	sgxintrin.h \
	shaintrin.h \
	smmintrin.h \
	tbmintrin.h \
	tmmintrin.h \
	tsxldtrkintrin.h \
	uintrintrin.h \
	vaesintrin.h \
	vpclmulqdqintrin.h \
	waitpkgintrin.h \
	wbnoinvdintrin.h \
	__wmmintrin_aes.h \
	wmmintrin.h \
	__wmmintrin_pclmul.h \
	x86gprintrin.h \
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
	arm_cmse.h \
	arm_neon.h
.  if ${MACHINE_CPU} == "aarch64"
INCS+=	arm64intr.h
.  endif
.elif ${MACHINE_CPU} == "mips"
INCS=	msa.h
.endif
INCS+=	stdatomic.h tgmath.h
INCSDIR=	/usr/include/clang-${CLANG_VERSION:R}
.endif # MKLLVM

.PATH:	${LLVM_SRCDIR}/include/llvm/IR
.PATH:	${LLVM_SRCDIR}/include/llvm/Frontend/OpenACC
.PATH:	${LLVM_SRCDIR}/include/llvm/Frontend/OpenMP

TABLEGEN_SRC=	Attributes.td Intrinsics.td Options.td ACC.td OMP.td

TABLEGEN_OUTPUT.Attributes.td= \
	llvm/IR/Attributes.inc|-gen-attrs

TABLEGEN_OUTPUT.Intrinsics.td= \
	llvm/IR/IntrinsicEnums.inc|-gen-intrinsic-enums \
	llvm/IR/IntrinsicImpl.inc|-gen-intrinsic-impl \
	llvm/IR/IntrinsicsAArch64.h|-gen-intrinsic-enums^-intrinsic-prefix=aarch64 \
	llvm/IR/IntrinsicsAMDGPU.h|-gen-intrinsic-enums^-intrinsic-prefix=amdgcn \
	llvm/IR/IntrinsicsARM.h|-gen-intrinsic-enums^-intrinsic-prefix=arm \
	llvm/IR/IntrinsicsBPF.h|-gen-intrinsic-enums^-intrinsic-prefix=bpf \
	llvm/IR/IntrinsicsHexagon.h|-gen-intrinsic-enums^-intrinsic-prefix=hexagon \
	llvm/IR/IntrinsicsMips.h|-gen-intrinsic-enums^-intrinsic-prefix=mips \
	llvm/IR/IntrinsicsNVPTX.h|-gen-intrinsic-enums^-intrinsic-prefix=nvvm \
	llvm/IR/IntrinsicsPowerPC.h|-gen-intrinsic-enums^-intrinsic-prefix=ppc \
	llvm/IR/IntrinsicsR600.h|-gen-intrinsic-enums^-intrinsic-prefix=r600 \
	llvm/IR/IntrinsicsRISCV.h|-gen-intrinsic-enums^-intrinsic-prefix=riscv \
	llvm/IR/IntrinsicsS390.h|-gen-intrinsic-enums^-intrinsic-prefix=s390 \
        llvm/IR/IntrinsicsVE.h|-gen-intrinsic-enums^-intrinsic-prefix=ve \
	llvm/IR/IntrinsicsWebAssembly.h|-gen-intrinsic-enums^-intrinsic-prefix=wasm \
	llvm/IR/IntrinsicsX86.h|-gen-intrinsic-enums^-intrinsic-prefix=x86 \
	llvm/IR/IntrinsicsXCore.h|-gen-intrinsic-enums^-intrinsic-prefix=xcore \

TABLEGEN_OUTPUT.OMP.td=	\
	llvm/Frontend/OpenMP/OMP.h.inc|--gen-directive-decl \
	llvm/Frontend/OpenMP/OMP.inc|--gen-directive-impl

TABLEGEN_OUTPUT.ACC.td=	\
	llvm/Frontend/OpenACC/ACC.h.inc|--gen-directive-decl \
	llvm/Frontend/OpenACC/ACC.inc|--gen-directive-impl

.if ${MKLLVM} != "no"
.PATH:	${CLANG_SRCDIR}/include/clang/AST \
	${CLANG_SRCDIR}/include/clang/Basic \
	${CLANG_SRCDIR}/include/clang/Driver \
	${CLANG_SRCDIR}/include/clang/StaticAnalyzer/Checkers \
	${CLANG_SRCDIR}/include/clang/Tooling/Syntax
CLANG_TABLEGEN_SRC=	\
	arm_cde.td \
	arm_fp16.td \
	arm_mve.td \
	arm_neon.td \
	arm_sve.td \
	Attr.td \
	Checkers.td \
	CommentCommands.td \
	CommentHTMLNamedCharacterReferences.td \
	CommentHTMLTags.td \
	CommentNodes.td \
	DeclNodes.td \
	Diagnostic.td \
	Nodes.td \
	PropertiesBase.td \
	riscv_vector.td \
	StmtDataCollectors.td \
	StmtNodes.td \
	TypeNodes.td \
	TypeProperties.td

CLANG_TABLEGEN_INCLUDES.Attr.td=	-I${CLANG_SRCDIR}/include
CLANG_TABLEGEN_OUTPUT.Attr.td= \
	clang/AST/AttrImpl.inc|-gen-clang-attr-impl \
	clang/AST/AttrNodeTraverse.inc|-gen-clang-attr-node-traverse \
	clang/AST/AttrTextNodeDump.inc|-gen-clang-attr-text-node-dump \
	clang/AST/Attrs.inc|-gen-clang-attr-classes \
	clang/AST/AttrVisitor.inc|-gen-clang-attr-ast-visitor \
	clang/Basic/AttrHasAttributeImpl.inc|-gen-clang-attr-has-attribute-impl \
	clang/Basic/AttrList.inc|-gen-clang-attr-list \
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

CLANG_TABLEGEN_OUTPUT.PropertiesBase.td= \
	clang/AST/AbstractBasicReader.inc|-gen-clang-basic-reader \
	clang/AST/AbstractBasicWriter.inc|-gen-clang-basic-writer
CLANG_TABLEGEN_INCLUDES.PropertiesBase.td=	-I${CLANG_SRCDIR}/include

CLANG_TABLEGEN_OUTPUT.TypeProperties.td= \
	clang/AST/AbstractTypeReader.inc|-gen-clang-type-reader \
	clang/AST/AbstractTypeWriter.inc|-gen-clang-type-writer
CLANG_TABLEGEN_INCLUDES.TypeProperties.td=	-I${CLANG_SRCDIR}/include

CLANG_TABLEGEN_OUTPUT.StmtNodes.td= \
	clang/AST/StmtNodes.inc|-gen-clang-stmt-nodes
CLANG_TABLEGEN_INCLUDES.StmtNodes.td=	-I${CLANG_SRCDIR}/include

CLANG_TABLEGEN_OUTPUT.StmtDataCollectors.td= \
	clang/AST/StmtDataCollectors.inc|-gen-clang-data-collectors

CLANG_TABLEGEN_OUTPUT.DeclNodes.td= \
	clang/AST/DeclNodes.inc|-gen-clang-decl-nodes
CLANG_TABLEGEN_INCLUDES.DeclNodes.td=	-I${CLANG_SRCDIR}/include

CLANG_TABLEGEN_OUTPUT.CommentNodes.td= \
	clang/AST/CommentNodes.inc|-gen-clang-comment-nodes
CLANG_TABLEGEN_INCLUDES.CommentNodes.td=	-I${CLANG_SRCDIR}/include

CLANG_TABLEGEN_OUTPUT.CommentCommands.td= \
	clang/AST/CommentCommandInfo.inc|-gen-clang-comment-command-info \
	clang/AST/CommentCommandList.inc|-gen-clang-comment-command-list

CLANG_TABLEGEN_OUTPUT.CommentHTMLTags.td= \
	clang/AST/CommentHTMLTags.inc|-gen-clang-comment-html-tags \
	clang/AST/CommentHTMLTagsProperties.inc|-gen-clang-comment-html-tags-properties

CLANG_TABLEGEN_OUTPUT.CommentHTMLNamedCharacterReferences.td= \
	clang/AST/CommentHTMLNamedCharacterReferences.inc|-gen-clang-comment-html-named-character-references

CLANG_TABLEGEN_INCLUDES.Nodes.td=	-I${CLANG_SRCDIR}/include
CLANG_TABLEGEN_OUTPUT.Nodes.td= \
	clang/Tooling/Syntax/NodeClasses.inc|-gen-clang-syntax-node-classes \
	clang/Tooling/Syntax/Nodes.inc|-gen-clang-syntax-node-list

CLANG_TABLEGEN_OUTPUT.TypeNodes.td= \
	clang/AST/TypeNodes.inc|-gen-clang-type-nodes
CLANG_TABLEGEN_INCLUDES.TypeNodes.td=	-I${CLANG_SRCDIR}/include

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

CLANG_TABLEGEN_INCLUDES.arm_cde.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.arm_cde.td= \
	clang/Basic/arm_cde_builtins.inc|-gen-arm-cde-builtin-def \
	clang/Basic/arm_cde_builtin_cg.inc|-gen-arm-cde-builtin-codegen \
	clang/Basic/arm_cde_builtin_sema.inc|-gen-arm-cde-builtin-sema \
	clang/Basic/arm_cde_builtin_aliases.inc|-gen-arm-cde-builtin-aliases

CLANG_TABLEGEN_INCLUDES.arm_sve.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.arm_sve.td= \
	clang/Basic/arm_sve_builtins.inc|-gen-arm-sve-builtins \
	clang/Basic/arm_sve_builtin_cg.inc|-gen-arm-sve-builtin-codegen \
	clang/Basic/arm_sve_sema_rangechecks.inc|-gen-arm-sve-sema-rangechecks \
	clang/Basic/arm_sve_typeflags.inc|-gen-arm-sve-typeflags

CLANG_TABLEGEN_INCLUDES.arm_neon.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.arm_neon.td= \
	clang/Basic/arm_neon.inc|-gen-arm-neon-sema \
	arm_neon.h|-gen-arm-neon

CLANG_TABLEGEN_INCLUDES.arm_fp16.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.arm_fp16.td= \
	clang/Basic/arm_fp16.inc|-gen-arm-neon-sema
CLANG_TABLEGEN_INCLUDES.arm_mve.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.arm_mve.td= \
	clang/Basic/arm_mve_builtins.inc|-gen-arm-mve-builtin-def \
	clang/Basic/arm_mve_builtin_aliases.inc|-gen-arm-mve-builtin-aliases \
	clang/Basic/arm_mve_builtin_cg.inc|-gen-arm-mve-builtin-codegen \
	clang/Basic/arm_mve_builtin_sema.inc|-gen-arm-mve-builtin-sema \

#CLANG_TABLEGEN_INCLUDES.riscv_vector.td=	-I${CLANG_SRCDIR}/include/clang/Basic
CLANG_TABLEGEN_OUTPUT.riscv_vector.td= \
	clang/Basic/riscv_vector_builtins.inc|-gen-riscv-vector-builtins \
	clang/Basic/riscv_vector_builtin_cg.inc|-gen-riscv-vector-builtin-codegen

CLANG_TABLEGEN_INCLUDES.Checkers.td=	\
	-I${CLANG_SRCDIR}/include \
	-I${CLANG_SRCDIR}/include/clang/StaticAnalyzer/Checkers
CLANG_TABLEGEN_OUTPUT.Checkers.td= \
	clang/StaticAnalyzer/Checkers/Checkers.inc|-gen-clang-sa-checkers

TABLEGEN_INCLUDES.Options.td=	-I${CLANG_SRCDIR}/include/clang/Driver
TABLEGEN_OUTPUT.Options.td= \
	clang/Driver/Options.inc|-gen-opt-parser-defs

.endif # MKLLVM

.include "${.PARSEDIR}/../tablegen.mk"

DPSRCS+=	llvm/Support/VCSRevision.h
DPSRC+=		llvm/Support/VCSRevision.h

llvm/Support/VCSRevision.h: ${LLVM_TOPLEVEL}/Makefile.inc
	mkdir -p ${.TARGET:H}
	printf '#define LLVM_REVISION "git-${LLVM_REVISION}"\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.if ${MKLLVM} != "no"
DPSRCS+=	clang/Basic/Version.inc
CLEANFILES+=	clang/Basic/Version.inc

clang/Basic/Version.inc: ${LLVM_TOPLEVEL}/Makefile.inc
	mkdir -p ${.TARGET:H}
	printf '#define CLANG_VERSION ${CLANG_VERSION}\n' > ${.TARGET}.tmp
	printf '#define CLANG_VERSION_STRING "${CLANG_VERSION}"\n' >> ${.TARGET}.tmp
	printf '#define CLANG_VERSION_MAJOR ${CLANG_VERSION:R:R}\n' >> ${.TARGET}.tmp
	printf '#define CLANG_VERSION_MINOR ${CLANG_VERSION:R:E}\n' >> ${.TARGET}.tmp
	printf '#define CLANG_VERSION_PATCHLEVEL ${CLANG_VERSION:R:R}\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}
.endif # MKLLVM

.ifndef HOST_BUILD

DPSRCS+=	llvm/Config/config.h llvm/Config/llvm-config.h
CLEANFILES+=	llvm/Config/config.h llvm/Config/llvm-config.h

NATIVE_LIBS=	-lm -lexecinfo

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
NATIVE_LLVM_ARCH=		X86
.endif

.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "aarch64eb"
NATIVE_LLVM_ARCH=		AArch64
.endif

.if !empty(MACHINE_ARCH:Mearm*) || !empty(MACHINE_ARCH:Marm*)
NATIVE_LLVM_ARCH=		ARM
.endif

.if !empty(MACHINE_ARCH:Mmips*)
NATIVE_LLVM_ARCH=		Mips
.endif

.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpc64"
NATIVE_LLVM_ARCH=		PowerPC
.endif

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
NATIVE_LLVM_ARCH=		Sparc
.endif

.if defined(NATIVE_LLVM_ARCH)
NATIVE_LLVM_ASMPARSER=		LLVMInitialize${NATIVE_LLVM_ARCH}AsmParser
NATIVE_LLVM_ASMPRINTER=		LLVMInitialize${NATIVE_LLVM_ARCH}AsmPrinter
NATIVE_LLVM_DISASSEMBLER=	LLVMInitialize${NATIVE_LLVM_ARCH}Disassembler
NATIVE_LLVM_TARGET=		LLVMInitialize${NATIVE_LLVM_ARCH}Target
NATIVE_LLVM_TARGETINFO=		LLVMInitialize${NATIVE_LLVM_ARCH}TargetInfo
NATIVE_LLVM_TARGETMC=		LLVMInitialize${NATIVE_LLVM_ARCH}TargetMC
.endif

llvm/Config/config.h: ${LLVM_TOPLEVEL}/Makefile.inc ${LLVM_TOPLEVEL}/config/llvm/Config/config.h.in
	mkdir -p ${.TARGET:H}
	${TOOL_SED} -e /LLVM_DEFAULT_TARGET_TRIPLE/d \
		    -e /LLVM_HOST_TRIPLE/d \
		    -e /LLVM_NATIVE_/d \
		    -e /LLVM_VERSION_/d \
		    -e /PACKAGE_VERSION/d -e 'x' \
	    < ${LLVM_TOPLEVEL}/config/llvm/Config/config.h.in \
	    > ${.TARGET}.tmp
	printf '#define LLVM_DEFAULT_TARGET_TRIPLE "${MACHINE_GNU_PLATFORM}"\n' >> ${.TARGET}.tmp
	printf '#define LLVM_HOST_TRIPLE "${MACHINE_GNU_PLATFORM}"\n' >> ${.TARGET}.tmp
.if defined(NATIVE_LLVM_ARCH)
	printf '#define LLVM_NATIVE_ARCH ${NATIVE_LLVM_ARCH}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_ASMPARSER ${NATIVE_LLVM_ASMPARSER}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_ASMPRINTER ${NATIVE_LLVM_ASMPRINTER}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_DISASSEMBLER ${NATIVE_LLVM_DISASSEMBLER}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_TARGET ${NATIVE_LLVM_TARGET}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_TARGETINFO ${NATIVE_LLVM_TARGETINFO}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_TARGETMC ${NATIVE_LLVM_TARGETMC}\n' >> ${.TARGET}.tmp
.endif
	printf '#define PACKAGE_VERSION "${LLVM_VERSION} (trunk ${LLVM_REVISION})"\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_MAJOR ${LLVM_VERSION:R:R}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_MINOR ${LLVM_VERSION:R:E}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_PATCH ${LLVM_VERSION:E}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_VERSION_STRING "${LLVM_VERSION}"\n' >> ${.TARGET}.tmp
	printf '#endif\n' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

llvm/Config/llvm-config.h: ${LLVM_TOPLEVEL}/Makefile.inc ${LLVM_TOPLEVEL}/config/llvm/Config/llvm-config.h.in
	mkdir -p ${.TARGET:H}
	${TOOL_SED} -e /LLVM_DEFAULT_TARGET_TRIPLE/d \
		    -e /LLVM_HOST_TRIPLE/d \
		    -e /LLVM_NATIVE_/d \
		    -e /LLVM_VERSION_/d \
		    -e /PACKAGE_VERSION/d -e 'x' \
	    < ${LLVM_TOPLEVEL}/config/llvm/Config/llvm-config.h.in \
	    > ${.TARGET}.tmp
	printf '#define LLVM_DEFAULT_TARGET_TRIPLE "${MACHINE_GNU_PLATFORM}"\n' >> ${.TARGET}.tmp
	printf '#define LLVM_HOST_TRIPLE "${MACHINE_GNU_PLATFORM}"\n' >> ${.TARGET}.tmp
.if defined(NATIVE_LLVM_ARCH)
	printf '#define LLVM_NATIVE_ARCH ${NATIVE_LLVM_ARCH}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_ASMPARSER ${NATIVE_LLVM_ASMPARSER}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_ASMPRINTER ${NATIVE_LLVM_ASMPRINTER}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_DISASSEMBLER ${NATIVE_LLVM_DISASSEMBLER}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_TARGET ${NATIVE_LLVM_TARGET}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_TARGETINFO ${NATIVE_LLVM_TARGETINFO}\n' >> ${.TARGET}.tmp
	printf '#define LLVM_NATIVE_TARGETMC ${NATIVE_LLVM_TARGETMC}\n' >> ${.TARGET}.tmp
.endif
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
	cp ${.CURDIR}/module.modulemap ${LLVM_INCLUDE_CONFIG}/llvm/module.modulemap
.endif

DPSRCS+=	llvm/Support/Extension.def
CLEANFILES+=	llvm/Support/Extension.def

llvm/Support/Extension.def:
	mkdir -p ${.TARGET:H}
	printf '//extension handlers\n#undef HANDLE_EXTENSION\n' > ${.TARGET}

includes depend dependall all: ${DPSRCS}

depend dependall: clean-modulecache

clean-modulecache:
	rm -rf ${LLVM_MODULE_CACHE}

.include <bsd.prog.mk>
