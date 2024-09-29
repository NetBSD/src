#	$NetBSD: Makefile,v 1.21 2024/09/29 16:53:05 kalvisd Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:S,c-family/,,:S,cp/,,:S,constraint.o,constraint.cc,:S,logic.o,logic.cc,:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:S,coroutines.o,coroutines.cc,:Nlibcpp.a:.o=.cc}
SRCS+=		main.cc ${PROG}-checksum.cc

# XXX
NOMAN=	1

CPPFLAGS+=	-I${DIST}/gcc/cp
CPPFLAGS.prefix.cc+=	-DPREFIX=\"/usr\"
CPPFLAGS.module.cc+=	-DTARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\" \
			-DHOST_MACHINE=\"${MACHINE_GNU_PLATFORM}\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.libcody"
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
#COPTS.c-common.cc+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/c-family ${DIST}/gcc/config ${DIST}/gcc/cp
.PATH: ${G_out_file:H}

