#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.3.16.2 2000/12/08 09:28:41 bouyer Exp $

COMPILE.S= $(CC) -x assembler-with-cpp -traditional-cpp \
		$(ASFLAGS) $(CPPFLAGS) $(INCPATH) -c -o $*.o

SRCS=   bootxx.c conf.c block_x.S
PROG=	bootxx
LIBS=	${LIBSA} ${LIBBUG}
DPADD=	${LIBS}
SRTOBJ=

.include "../Makefile.booters"

.include <bsd.prog.mk>
