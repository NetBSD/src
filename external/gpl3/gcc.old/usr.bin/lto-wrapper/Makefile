#	$NetBSD: Makefile,v 1.3.2.1 2018/11/26 01:52:09 pgoyette Exp $

NOMAN=1
PROG=		lto-wrapper
SRCS=		lto-wrapper.c \
		collect-utils.c
CPPFLAGS+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.common-target"
.include "../Makefile.libcpp"

COPTS.lto-wrapper.c=	-Wno-stack-protector


DPADD+=                 ${LIBIBERTYOBJ}/libiberty.a
LDADD+=                 ${LIBIBERTYOBJ}/libiberty.a

# VER_CPPFLAGS from Makefile.inc

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc ${DIST}/gcc/c ${DIST}/gcc/c-family ${DIST}/gcc/config
.PATH: ${G_out_file:H}
