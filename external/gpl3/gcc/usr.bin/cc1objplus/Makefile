#	$NetBSD: Makefile,v 1.3.6.1 2023/10/08 14:35:21 martin Exp $

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

BUILDSYMLINKS+= ${DIST}/gcc/objc/objc-act.c objcp-act.c \
		${DIST}/gcc/objc/objc-lang.c objcp-lang.c
	
MYOBJS=		${G_OBJCXX_OBJS}
MY2OBJS=	${MYOBJS:S,objcp/,,:S,objc/,,:S,cp/,,:S,c-family/,,:S,c/,,}
PROG=		cc1objplus
SRCS=		${MY2OBJS:S,known-headers.o,known-headers.cc,:S,c-spellcheck.o,c-spellcheck.cc,:S,constraint.o,constraint.cc,:S,logic.o,logic.cc,:S,coroutines.o,coroutines.cc,:Nlibcpp.a:.o=.c}
SRCS+=		main.c ${PROG}-checksum.c


CPPFLAGS+=	-I${DIST}/gcc/objc
CPPFLAGS+=	-I${DIST}/gcc/objcp
CPPFLAGS+=	-I${DIST}/gcc/cp
CPPFLAGS+=	-DOBJCPLUS
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
		${MY2OBJS} \
		checksum-options

.include "../Makefile.checksum"

# XXX: like cc1plus
CFLAGS+=-Wno-error=stack-protector

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

.PATH: ${DIST}/gcc/objcp ${DIST}/gcc/objc ${DIST}/gcc/c ${DIST}/gcc/c-family
.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${DIST}/gcc/config
.PATH: ${G_out_file:H}
