#	$NetBSD: Makefile,v 1.4.8.1 1997/03/12 14:05:00 is Exp $

SA_PROG= netboot
SRCS=   boot.c conf.c version.c
OBJS=   ${SRCS:S/.c/.o/g}

# Using local rules for these
OBJS+=	dev_net.o globals.o

DEFS= -DSTANDALONE

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

dev_net.o : ${S}/lib/libsa/dev_net.c
	${CC} ${CFLAGS} ${CPPFLAGS} -DSUN_BOOTPARAMS -c ${.IMPSRC}

globals.o : ${S}/lib/libsa/globals.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c ${.IMPSRC}

