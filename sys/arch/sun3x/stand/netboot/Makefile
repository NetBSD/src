#	$NetBSD: Makefile,v 1.2.4.1 1997/09/06 18:44:28 thorpej Exp $

SA_PROG= netboot
SRC1=   boot.c conf.c version.c
OBJS+= ${SRC1:.c=.o}

# Using local rule for this
OBJS+= dev_net.o

# Build these locally too
SRC2= bootparam.c globals.c nfs.c rpc.c
OBJS+= ${SRC2:.c=.o}

SRCS= ${SRC1} ${SRC2}

all:	${SA_PROG}.bin

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa

# How about -DSUPPORT_BOOTP instead?
dev_net.o : ${S}/lib/libsa/dev_net.c
	${COMPILE.c} -DSUN_BOOTPARAMS ${.IMPSRC}

