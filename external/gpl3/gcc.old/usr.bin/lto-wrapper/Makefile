#	$NetBSD: Makefile,v 1.3.4.2 2020/04/13 08:02:10 martin Exp $

NOMAN=1
PROG=		lto-wrapper
# XXX pullout from LTO_WRAPPER_OBJS
SRCS=		lto-wrapper.c \
		collect-utils.c \
		ggc-none.o

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
