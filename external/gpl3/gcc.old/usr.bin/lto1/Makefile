#	$NetBSD: Makefile,v 1.4 2018/11/11 22:50:13 mrg Exp $

PROG=		lto1
SRCS=		lto-partition.c \
		lto-object.c \
		lto.c \
		lto-lang.c \
		lto-symtab.c \
		attribs.c \
		main.c

CPPFLAGS+=	-DPREFIX=\"/usr\"

COPTS.lto.c+=		-Wno-stack-protector
COPTS.lto-lang.c+=	-Wno-stack-protector

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.common"
.include "../Makefile.common-target"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"
.include "../Makefile.libiberty"

.if ${MACHINE_ARCH} == "vax"
COPTS.lto-lang.c+=-O0
COPTS.lto-symtab.c+=-O0
.endif

LDADD+= ${LIBIBERTYOBJ}/libiberty.a
DPADD+= ${LIBIBERTYOBJ}/libiberty.a
LDADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/lto  ${DIST}/gcc # ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
