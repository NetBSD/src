#	$Id: Makefile,v 1.14 1993/08/02 17:18:41 mycroft Exp $

PROG=	ed
CFLAGS+=-DVI_BANG -DDES -DGNU_REGEX
SRCS=	ed.c re.c buf.c cbc.c
LDADD=	-lgnuregex

.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

LINKS=  ${BINDIR}/ed ${BINDIR}/red
MLINKS= ed.1 red.1

.include <bsd.prog.mk>
