#	$NetBSD: Makefile,v 1.7 2024/06/30 16:15:04 christos Exp $

NOMAN=1
PROG=		lto-wrapper
# XXX pullout from LTO_WRAPPER_OBJS
SRCS=		lto-wrapper.cc \
		collect-utils.cc \
		ggc-none.cc

BINDIR=		/usr/libexec

CPPFLAGS+=	-DPREFIX=\"/usr\" -I${GCCARCH} -I${G_ALL_CPPFLAGS}

.include "../Makefile.prog"
.include "../Makefile.target-defines"
.include "../Makefile.common-target"
.include "../Makefile.common"
.include "../Makefile.libcpp"
.include "../Makefile.backtrace"
.include "../Makefile.libiberty"

COPTS.lto-wrapper.cc=	-Wno-error=stack-protector

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
