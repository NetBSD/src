#	$NetBSD: Makefile,v 1.17.6.1 2023/10/08 14:35:22 martin Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:S,c-family/,,:S,cp/,,:S,constraint.o,constraint.cc,:S,logic.o,logic.cc,:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:S,coroutines.o,coroutines.cc,:Nlibcpp.a:.o=.c}
SRCS+=		main.c ${PROG}-checksum.c

# XXX
NOMAN=	1

CPPFLAGS+=	-I${DIST}/gcc/cp
CPPFLAGS.prefix.c+=     -DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"

CHECKSUM_OBJS= ${LIBBACKTRACEOBJ}/libbacktrace.a \
               ${COMMONTARGETOBJ}/libcommon-target.a \
               ${COMMONOBJ}/libcommon.a \
               ${LIBCPPOBJ}/libcpp.a \
               ${LIBDECNUMBEROBJ}/libdecnumber.a \
               ${LIBIBERTYOBJ}/libiberty.a \
               ${G_CXX_OBJS:S,c-family/,,:S,c/,,} \
               checksum-options

.include "../Makefile.checksum"

# XXX check this out
# LDFLAGS+=   -rdynamic

CFLAGS+=	-Wno-error=stack-protector

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
COPTS.c-common.c+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/c-family ${DIST}/gcc/config ${DIST}/gcc/cp
.PATH: ${G_out_file:H}

