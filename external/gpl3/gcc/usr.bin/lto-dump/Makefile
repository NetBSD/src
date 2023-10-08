#	$NetBSD: Makefile,v 1.1.6.1 2023/10/08 14:35:19 martin Exp $

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

LDADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBIBERTY} ${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/lto ${DIST}/gcc ${DIST}/gcc/doc
