#	$NetBSD: Makefile,v 1.6 2018/11/11 22:50:06 mrg Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:S,c-family/,,:S,c/,,:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c
CPPFLAGS+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.common"
.include "../Makefile.common-target"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"
.include "../Makefile.libiberty"

CHECKSUM_OBJS=	${BACKENDOBJ}/libbackend.a \
		main.o \
		${LIBBACKTRACEOBJ}/libbacktrace.a \
		${COMMONOBJ}/libcommon.a \
		${COMMONTARGETOBJ}/libcommon-target.a \
		${LIBCPPOBJ}/libcpp.a \
		${LIBDECNUMBEROBJ}/libdecnumber.a \
		${LIBIBERTYOBJ}/libiberty.a \
		${G_C_OBJS:S,c-family/,,:S,c/,,} \
		checksum-options

.include "../Makefile.checksum"

# XXX check this out, probably need it for plugins?
# LDFLAGS+=	-rdynamic

# VER_CPPFLAGS from Makefile.inc
CPPFLAGS.c-cppbuiltin.c=	${VER_CPPFLAGS}
c-cppbuiltin.c: ${GCCARCH}/defs.mk

COPTS.c-ada-spec.c=	-Wno-stack-protector
COPTS.c-aux-info.c=	-Wno-stack-protector
COPTS.c-common.c=	-Wno-stack-protector
COPTS.c-cppbuiltin.c=	-Wno-stack-protector
COPTS.c-decl.c=		-Wno-stack-protector
COPTS.c-format.c=	-Wno-stack-protector
COPTS.c-format.h=	-Wno-stack-protector
COPTS.c-lex.c=		-Wno-stack-protector
COPTS.c-opts.c=		-Wno-stack-protector
COPTS.c-ppoutput.c=	-Wno-stack-protector
COPTS.c-pragma.c=	-Wno-stack-protector
COPTS.c-typeck.c=	-Wno-stack-protector

CPPFLAGS.default-c.c+=	-I${BACKENDOBJ}

.if ${MACHINE_CPU} == "sh3"
BUILDSYMLINKS+= ${GNUHOSTDIST}/gcc/config/sh/sh-c.cc sh-c.c
.endif

.if ${MACHINE_CPU} == "vax"
COPTS.c-decl.c+=-O0
COPTS.c-typeck.c+=-O0
COPTS.c-array-notation.c+=-O0
COPTS.c-common.c+=-O0
COPTS.c-ada-spec.c+=-O0
COPTS.cilk.c+=-O0
COPTS.c-ubsan.c+=-O0
COPTS.cc1-checksum.c+=-O0
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
