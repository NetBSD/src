#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.9 2011/01/02 05:30:12 tsutsui Exp $

COMPILE.S= $(CC) -x assembler-with-cpp -traditional-cpp \
		$(ASFLAGS) $(CPPFLAGS) $(INCPATH) -c -o $*.o

SRCS=   bootxx.c conf.c block_x.S
PROG=	bootxx
LIBS=	${LIBSA} ${LIBKERN} ${LIBBUG}
DPADD=	${LIBS}
SRTOBJ=

.include "../Makefile.booters"

.include <bsd.prog.mk>
