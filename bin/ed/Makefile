#	$Id: Makefile,v 1.18 1993/11/11 01:29:52 jtc Exp $

PROG=	ed
CFLAGS+=-DVI_BANG -DDES
SRCS=	ed.c re.c buf.c cbc.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
