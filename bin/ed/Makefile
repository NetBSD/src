PROG=	ed
CFLAGS+=-DBACKWARDS #-DDES
SRCS=	 buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt -lcompat
DPADD+= ${LIBCRYPT} ${LIBCOMPAT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
