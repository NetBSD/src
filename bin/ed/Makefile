PROG=	ed
CFLAGS+=-DVI_BANG -DDES -DGNU_REGEX
SRCS=	ed.c re.c buf.c cbc.c
LDADD=	-lgnuregex -lcrypt
LINKS=  ${BINDIR}/ed ${BINDIR}/red
MLINKS= ed.1 red.1

.include <bsd.prog.mk>
