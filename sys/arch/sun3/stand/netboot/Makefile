#	$NetBSD: Makefile,v 1.7 1997/03/14 21:28:38 gwr Exp $

SA_PROG= netboot
SRCS=   boot.c conf.c version.c
OBJS=   ${SRCS:S/.c/.o/g}

# Using local rules for these
OBJS+=	dev_net.o globals.o

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

dev_net.o : ${S}/lib/libsa/dev_net.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c ${.IMPSRC}

globals.o : ${S}/lib/libsa/globals.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c ${.IMPSRC}

