#	$NetBSD: Makefile,v 1.2 2023/02/20 02:12:36 mrg Exp $

PROG=		lto-dump
# XXX pullout from LTO_DUMP_OBJS
SRCS=		lto-lang.c \
		lto-object.c \
		attribs.c \
		lto-partition.c \
		lto-symtab.c \
		lto-dump.c \
		lto-common.c \
		main.c

BINDIR=		/usr/bin

CPPFLAGS+=	-DPREFIX=\"/usr\"

COPTS.lto-lang.c+=	-Wno-stack-protector
COPTS.lto-common.c+=	-Wno-stack-protector

.include "../Makefile.backend"
.include "../Makefile.target-defines"
.include "../Makefile.libcpp"
.include "../Makefile.backtrace"
.include "../Makefile.libdecnumber"

.if ${MACHINE_ARCH} == "vax"
COPTS.lto-lang.c+=-O0
COPTS.lto-symtab.c+=-O0
.endif

LDADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/lto ${DIST}/gcc ${DIST}/gcc/doc
