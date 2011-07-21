#	$NetBSD: Makefile,v 1.4 2011/07/21 03:13:32 mrg Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c

CPPFLAGS+=	-I${DIST}/gcc/cp
CPPFLAGS.prefix.c+=     -DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"

# VER_CPPFLAGS from Makefile.inc
CPPFLAGS.c-cppbuiltin.c=	${VER_CPPFLAGS}

# YUCK.  but see timevar.h:POP_TIMEVAR_AND_RETURN
CFLAGS+=	-Wno-error # not good enough: -Wno-return-type

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ}

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${G_out_file:H}
