# $NetBSD: bsd.clang-analyze.mk,v 1.1 2011/08/27 18:35:20 joerg Exp $

.ifndef CLANG_ANALYZE_SRCS

CLANG_ANALYZE_FLAGS+=	-Xclang -analyze \
			-Xclang -analyzer-store=region \
			-Xclang -analyzer-opt-analyze-nested-blocks \
			-Xclang -analyzer-eagerly-assume \
			-Xclang -analyzer-checker=core \
			-Xclang -analyzer-checker=deadcode \
			-Xclang -analyzer-checker=security \
			-Xclang -analyzer-checker=unix \
			-fsyntax-only

.SUFFIXES: .c .cc .cpp .cxx .C .clang-analyzer

CLANG_ANALYZE_CFLAGS=		${CFLAGS:N-Wa,--fatal-warnings}
CLANG_ANALYZE_CXXFLAGS=	${CXXFLAGS:N-Wa,--fatal-warnings}

.c.clang-analyzer:
	${TOOL_CC.clang} ${CLANG_ANALYZE_FLAGS} \
	    ${CLANG_ANALYZE_CFLAGS} ${CPPFLAGS} \
	    ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
.cc.clang-analyzer .cpp.clang-analyzer .cxx.clang-analyzer .C.clang-analyzer:
	${TOOL_CXX.clang} ${CLANG_ANALYZE_FLAGS} \
	    ${CLANG_ANALYZE_CXXFLAGS} ${CPPFLAGS} \
	    ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

CLANG_ANALYZE_SRCS= \
	${SRCS:M*.[cC]} ${SRCS:M*.cc} \
	${SRCS:M*.cpp} ${SRCS:M*.cxx} \
	${DPSRCS:M*.[cC]} ${DPSRCS:M*.cc} \
	${DPSRCS:M*.cpp} ${DPSRCS:M*.cxx}
.if !empty(CLANG_ANALYZE_SRCS)
CLANG_ANALYZE_OUTPUT=	${CLANG_ANALYZE_SRCS:R:S,$,.clang-analyzer,}
.endif

analyze: ${CLANG_ANALYZE_OUTPUT}

.endif
