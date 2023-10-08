#	$NetBSD: Makefile,v 1.20.6.1 2023/10/08 14:35:22 martin Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:S,c-family/,,:S,c/,,:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:Nlibcpp.a:.o=.c}
SRCS+=		main.c ${PROG}-checksum.c
CPPFLAGS+=	-DPREFIX=\"/usr\"

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
		${G_C_OBJS:S,c-family/,,:S,c/,,} \
		checksum-options

.include "../Makefile.checksum"

# XXX check this out, probably need it for plugins?
# LDFLAGS+=	-rdynamic

COPTS.c-ada-spec.c=	-Wno-stack-protector
COPTS.c-attribs.c=	-Wno-stack-protector
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

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
COPTS.c-common.c+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
