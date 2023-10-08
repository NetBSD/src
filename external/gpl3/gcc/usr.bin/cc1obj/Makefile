#	$NetBSD: Makefile,v 1.18.6.1 2023/10/08 14:35:23 martin Exp $

MYOBJS=		${G_OBJC_OBJS} ${G_C_AND_OBJC_OBJS} 
PROG=		cc1obj
SRCS=		${MYOBJS:S,objc/,,:S,c-family/,,:S,c/,,:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:Nlibcpp.a:.o=.c}
SRCS+=		main.c ${PROG}-checksum.c

CPPFLAGS+=	-I${DIST}/gcc/objc
CPPFLAGS.prefix.c+=	-DPREFIX=\"/usr\"

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

COPTS.c-ada-spec.c+=			-Wno-stack-protector
COPTS.c-attribs.c+=			-Wno-stack-protector
COPTS.c-aux-info.c+=			-Wno-stack-protector
COPTS.c-common.c+=			-Wno-stack-protector
COPTS.c-cppbuiltin.c+=			-Wno-stack-protector
COPTS.c-decl.c+=			-Wno-stack-protector
COPTS.c-format.c+=			-Wno-stack-protector
COPTS.c-lex.c+=				-Wno-stack-protector
COPTS.c-ppoutput.c+=			-Wno-stack-protector
COPTS.c-pragma.c+=			-Wno-stack-protector
COPTS.c-typeck.c+=			-Wno-stack-protector
COPTS.objc-act.c+=			-Wno-stack-protector
COPTS.objc-gnu-runtime-abi-01.c+=	-Wno-stack-protector
COPTS.objc-next-runtime-abi-01.c+=	-Wno-stack-protector

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"
COPTS.c-common.c+=-O3
.endif

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/objc ${DIST}/gcc/c ${DIST}/gcc/c-family
.PATH: ${DIST}/gcc ${DIST}/gcc/config
.PATH: ${G_out_file:H}
