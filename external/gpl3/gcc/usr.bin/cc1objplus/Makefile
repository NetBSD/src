#	$NetBSD: Makefile,v 1.4 2023/08/01 05:57:54 mrg Exp $

# XXX: Pull this (G_OBJCXX_OBJS) out with mknative-gcc.  Needs some
# minor re-org of cc1plus and perhaps cc1obj.  G_CXX_AND_OBJCXX_OBJS
# is reverse-derived from G_CXX_OBJS for now - when G_OBJCXX_OBJS is
# pulled out by mknative-gcc, it will include G_CXX_AND_OBJCXX_OBJS.
G_CXX_AND_OBJCXX_OBJS=	${G_CXX_OBJS:S,cp-lang.o,,:S,c-family/stub-objc.o,,}
G_OBJCXX_OBJS= \
	objcp-act.o \
	objcp-lang.o \
	objcp/objcp-decl.o \
	objcp/objc-runtime-shared-support.o \
	objcp/objc-gnu-runtime-abi-01.o \
	objcp/objc-next-runtime-abi-01.o \
	objcp/objc-next-runtime-abi-02.o \
	objcp/objc-encoding.o \
	objcp/objc-map.o \
	${G_CXX_AND_OBJCXX_OBJS}

BUILDSYMLINKS+= ${DIST}/gcc/objc/objc-act.cc objcp-act.cc \
		${DIST}/gcc/objc/objc-lang.cc objcp-lang.cc
	
MYOBJS=		${G_OBJCXX_OBJS}
MY2OBJS=	${MYOBJS:S,objcp/,,:S,objc/,,:S,cp/,,:S,c-family/,,:S,c/,,}
PROG=		cc1objplus
SRCS=		${MY2OBJS:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:S,constraint.o,constraint.cc,:S,logic.o,logic.cc,:S,coroutines.o,coroutines.cc,:Nlibcpp.a:.o=.cc}
SRCS+=		main.cc ${PROG}-checksum.cc


CPPFLAGS+=	-I${DIST}/gcc/objc
CPPFLAGS+=	-I${DIST}/gcc/objcp
CPPFLAGS+=	-I${DIST}/gcc/cp
CPPFLAGS+=	-DOBJCPLUS
CPPFLAGS.prefix.cc+=	-DPREFIX=\"/usr\"
CPPFLAGS.module.cc+=	-DTARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\" \
			-DHOST_MACHINE=\"${MACHINE_GNU_PLATFORM}\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.libcody"
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
		${MY2OBJS} \
		checksum-options

.include "../Makefile.checksum"

# XXX: like cc1plus
CFLAGS+=-Wno-error=stack-protector

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
COPTS.decl.c+=-O0
.endif

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
#COPTS.c-common.cc+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/objcp ${DIST}/gcc/objc ${DIST}/gcc/c ${DIST}/gcc/c-family
.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/config
.PATH: ${G_out_file:H}
