#	$NetBSD: Makefile,v 1.22 2024/09/29 16:45:29 kalvisd Exp $

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

COPTS.c-ada-spec.cc+=			-Wno-error=stack-protector
COPTS.c-attribs.cc+=			-Wno-error=stack-protector
COPTS.c-aux-info.cc+=			-Wno-error=stack-protector
COPTS.c-common.cc+=			-Wno-error=stack-protector
COPTS.c-cppbuiltin.cc+=			-Wno-error=stack-protector
COPTS.c-decl.cc+=			-Wno-error=stack-protector
COPTS.c-format.cc+=			-Wno-error=stack-protector
COPTS.c-lex.cc+=			-Wno-error=stack-protector
COPTS.c-omp.cc+=			-Wno-error=stack-protector
COPTS.c-ppoutput.cc+=			-Wno-error=stack-protector
COPTS.c-pragma.cc+=			-Wno-error=stack-protector
COPTS.c-pretty-print.cc+=		-Wno-error=stack-protector
COPTS.c-typeck.cc+=			-Wno-error=stack-protector
COPTS.objc-act.cc+=			-Wno-error=stack-protector
COPTS.objc-gnu-runtime-abi-01.cc+=	-Wno-error=stack-protector
COPTS.objc-next-runtime-abi-01.cc+=	-Wno-error=stack-protector

.if ${MACHINE_ARCH} == "vax" && 0
#COPTS.c-decl.c+=-O0
#COPTS.c-typeck.c+=-O0
#COPTS.c-array-notation.c+=-O0
#COPTS.c-common.c+=-O0
#COPTS.c-ada-spec.c+=-O0
#COPTS.cilk.c+=-O0
#COPTS.c-ubsan.c+=-O0
#COPTS.cc1obj-checksum.c+=-O0
.endif

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
#COPTS.cc-common.cc+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/objc ${DIST}/gcc/c ${DIST}/gcc/c-family
.PATH: ${DIST}/gcc ${DIST}/gcc/config
.PATH: ${G_out_file:H}
