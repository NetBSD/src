#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.10 2011/01/21 15:59:07 joerg Exp $

COMPILE.S= $(CC) -x assembler-with-cpp \
		$(ASFLAGS) $(CPPFLAGS) $(INCPATH) -c -o $*.o

SRCS=   bootxx.c conf.c block_x.S
PROG=	bootxx
LIBS=	${LIBSA} ${LIBKERN} ${LIBBUG}
DPADD=	${LIBS}
SRTOBJ=

.include "../Makefile.booters"

.include <bsd.prog.mk>
