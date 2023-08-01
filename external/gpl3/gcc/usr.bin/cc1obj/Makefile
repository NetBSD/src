#	$NetBSD: Makefile,v 1.19 2023/08/01 05:57:54 mrg Exp $

MYOBJS=		${G_OBJC_OBJS} ${G_C_AND_OBJC_OBJS} 
PROG=		cc1obj
SRCS=		${MYOBJS:S,objc/,,:S,c-family/,,:S,c/,,:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:Nlibcpp.a:.o=.cc}
SRCS+=		main.cc ${PROG}-checksum.cc

CPPFLAGS+=	-I${DIST}/gcc/objc
CPPFLAGS.prefix.cc+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"

CHECKSUM_OBJS=	${BACKENDOBJ}/libbackend.a \
		main.o \
		${LIBBACKTRACEOBJ}/libbacktrace.a \
		${COMMONTARGETOBJ}/libcommon-target.a \
		${COMMONOBJ}/libcommon.a \
		${LIBCPPOBJ}/libcpp.a \
		${LIBDECNUMBEROBJ}/libdecnumber.a \
		${LIBIBERTYOBJ}/libiberty.a \
		${MYOBJS:S,objc/,,:S,c-family/,,:S,c/,,} \
		checksum-options

.include "../Makefile.checksum"

# XXX check this out
# LDFLAGS+=	-rdynamic

.if 0
COPTS.c-ada-spec.cc+=			-Wno-stack-protector
COPTS.c-attribs.cc+=			-Wno-stack-protector
COPTS.c-aux-info.cc+=			-Wno-stack-protector
COPTS.c-common.cc+=			-Wno-stack-protector
COPTS.c-cppbuiltin.cc+=			-Wno-stack-protector
COPTS.c-decl.cc+=			-Wno-stack-protector
COPTS.c-format.cc+=			-Wno-stack-protector
COPTS.c-lex.cc+=			-Wno-stack-protector
COPTS.c-ppoutput.cc+=			-Wno-stack-protector
COPTS.c-pragma.cc+=			-Wno-stack-protector
COPTS.c-typeck.cc+=			-Wno-stack-protector
COPTS.objc-act.cc+=			-Wno-stack-protector
COPTS.objc-gnu-runtime-abi-01.cc+=	-Wno-stack-protector
COPTS.objc-next-runtime-abi-01.cc+=	-Wno-stack-protector
.endif

.if ${MACHINE_ARCH} == "vax" && 0
COPTS.c-decl.c+=-O0
COPTS.c-typeck.c+=-O0
COPTS.c-array-notation.c+=-O0
COPTS.c-common.c+=-O0
COPTS.c-ada-spec.c+=-O0
COPTS.cilk.c+=-O0
COPTS.c-ubsan.c+=-O0
COPTS.cc1obj-checksum.c+=-O0
.endif

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
#COPTS.cc-common.cc+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/objc ${DIST}/gcc/c ${DIST}/gcc/c-family
.PATH: ${DIST}/gcc ${DIST}/gcc/config
.PATH: ${G_out_file:H}
