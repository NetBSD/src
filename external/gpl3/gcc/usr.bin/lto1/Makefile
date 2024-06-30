#	$NetBSD: Makefile,v 1.9 2024/06/30 16:15:04 christos Exp $

PROG=		lto1
SRCS=		attribs.cc \
		lto-common.cc \
		lto-lang.cc \
		lto-object.cc \
		lto-partition.cc \
		lto-symtab.cc \
		lto.cc \
		main.cc

CPPFLAGS+=	-DPREFIX=\"/usr\"

COPTS.lto.cc+=		-Wno-error=stack-protector
COPTS.lto-lang.cc+=	-Wno-error=stack-protector
COPTS.lto-common.cc+=	-Wno-error=stack-protector

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"

.if ${MACHINE_ARCH} == "vax" && 0
COPTS.lto-lang.c+=-O0
COPTS.lto-symtab.c+=-O0
.endif

LDADD+= ${LIBIBERTYOBJ}/libiberty.a
DPADD+= ${LIBIBERTYOBJ}/libiberty.a
LDADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/lto  ${DIST}/gcc # ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
