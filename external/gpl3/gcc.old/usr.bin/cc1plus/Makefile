#	$NetBSD: Makefile,v 1.6 2018/11/11 22:50:07 mrg Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:S,c-family/,,:S,cp/,,:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c

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

## VER_CPPFLAGS from Makefile.inc
#CPPFLAGS.c-cppbuiltin.c=	${VER_CPPFLAGS}
#c-cppbuiltin.c: ${GCCARCH}/defs.mk

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


LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}

