#	$NetBSD: Makefile,v 1.5 1997/03/11 18:29:30 gwr Exp $

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

