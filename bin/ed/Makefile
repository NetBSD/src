#	$Id: Makefile,v 1.17 1993/10/28 05:36:31 cgd Exp $

PROG=	ed
CFLAGS+=-DVI_BANG -DDES
SRCS=	ed.c re.c buf.c cbc.c

LDADD+= -lcrypt -lgnuregex
DPADD+= ${LIBCRYPT} /usr/lib/libgnuregex.a

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
