#	$NetBSD: Makefile,v 1.2 2003/09/23 09:04:41 matt Exp $

PROG=		cc1plus
SRCS=		${G_CXX_OBJS:Nlibcpp.a:.o=.c} ${G_CXX_C_OBJS:.o=.c} main.o

CPPFLAGS+=	-I${DIST}/gcc/cp

.include "../Makefile.backend"
.include "../Makefile.libcpp"

# YUCK.  but see timevar.h:POP_TIMEVAR_AND_RETURN
CFLAGS+=	-Wno-error # not good enough: -Wno-return-type

.PATH: ${DIST}/gcc/cp ${DIST}/gcc ${G_out_file:H}
