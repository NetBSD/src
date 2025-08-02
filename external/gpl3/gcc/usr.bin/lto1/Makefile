#	$NetBSD: Makefile,v 1.8.2.2 2025/08/02 05:26:15 perseant Exp $

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

LDADD+= ${LIBIBERTYOBJ}/libiberty.a
DPADD+= ${LIBIBERTYOBJ}/libiberty.a
LDADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/lto  ${DIST}/gcc # ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
