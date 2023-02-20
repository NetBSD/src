#	$NetBSD: Makefile,v 1.9 2023/02/20 02:12:36 mrg Exp $

NOMAN=1
PROG=		lto-wrapper
# XXX pullout from LTO_WRAPPER_OBJS
SRCS=		lto-wrapper.c \
		collect-utils.c \
		ggc-none.c

BINDIR=		/usr/libexec

CPPFLAGS+=	-DPREFIX=\"/usr\" -I${GCCARCH} -I${G_ALL_CPPFLAGS}

.include "../Makefile.prog"
.include "../Makefile.target-defines"
.include "../Makefile.common-target"
.include "../Makefile.common"
.include "../Makefile.libcpp"
.include "../Makefile.backtrace"
.include "../Makefile.libiberty"

COPTS.lto-wrapper.c=	-Wno-stack-protector

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
