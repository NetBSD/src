#	$NetBSD: Makefile,v 1.2 2006/07/01 19:06:51 mrg Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c

CPPFLAGS+=	-I${DIST}/gcc/cp
CPPFLAGS.prefix.c+=     -DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"

# YUCK.  but see timevar.h:POP_TIMEVAR_AND_RETURN
CFLAGS+=	-Wno-error # not good enough: -Wno-return-type

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${G_out_file:H}
