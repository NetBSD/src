PROG=   ed
CFLAGS+=-DVI_BANG -DDES -DBACKWARDS
SRCS=   ed.c re.c buf.c cbc.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>

