#	$NetBSD: Makefile,v 1.10 2020/08/19 07:48:03 mrg Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:S,c-family/,,:S,cp/,,:S,constraint.o,constraint.cc,:S,logic.o,logic.cc,:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:Nlibcpp.a:.o=.c}
SRCS+=		main.c ${PROG}-checksum.c

# XXX
NOMAN=	1

CPPFLAGS+=	-I${DIST}/gcc/cp
CPPFLAGS.prefix.c+=     -DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.common"
.include "../Makefile.common-target"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"
.include "../Makefile.libiberty"

CHECKSUM_OBJS= ${LIBBACKTRACEOBJ}/libbacktrace.a \
               ${COMMONOBJ}/libcommon.a \
               ${COMMONTARGETOBJ}/libcommon-target.a \
               ${LIBCPPOBJ}/libcpp.a \
               ${LIBDECNUMBEROBJ}/libdecnumber.a \
               ${LIBIBERTYOBJ}/libiberty.a \
               ${G_CXX_OBJS:S,c-family/,,:S,c/,,} \
               checksum-options

.include "../Makefile.checksum"

# XXX check this out
# LDFLAGS+=   -rdynamic

# YUCK.  but see timevar.h:POP_TIMEVAR_AND_RETURN
CFLAGS+=	-Wno-error # not good enough: -Wno-return-type
COPTS+=		-Wno-stack-protector

.if ${MACHINE_ARCH} == "vax"
COPTS.call.c+=-O0
COPTS.decl.c+=-O0
COPTS.typeck2.c+=-O0
COPTS.class.c+=-O0
COPTS.typeck.c+=-O0
COPTS.init.c+=-O0
COPTS.semantics.c+=-O0
COPTS.mangle.c+=-O0
COPTS.constexpr.c+=-O0
COPTS.c-common.c+=-O0
COPTS.c-ada-spec.c+=-O0
COPTS.cilk.c+=-O0
COPTS.c-ubsan.c+=-O0
COPTS.cc1plus-checksum.c+=-O0
.endif

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
COPTS.c-common.c+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}

