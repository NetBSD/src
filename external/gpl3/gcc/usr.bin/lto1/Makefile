#	$NetBSD: Makefile,v 1.7.6.1 2023/10/08 14:35:23 martin Exp $

PROG=		lto1
SRCS=		attribs.c \
		lto-common.c \
		lto-lang.c \
		lto-object.c \
		lto-partition.c \
		lto-symtab.c \
		lto.c \
		main.c

CPPFLAGS+=	-DPREFIX=\"/usr\"

COPTS.lto.c+=		-Wno-stack-protector
COPTS.lto-lang.c+=	-Wno-stack-protector
COPTS.lto-common.c+=	-Wno-stack-protector

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
