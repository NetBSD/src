#	$NetBSD: Makefile,v 1.4.6.1 1998/01/27 02:35:43 gwr Exp $

SA_PROG= ufsboot
SRCS= boot.c conf.c
OBJS= ${SRCS:.c=.o}

# Using local rule for this
OBJS+= ufs.o

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

ufs.o : ${S}/lib/libsa/ufs.c
	${COMPILE.c} -DCOMPAT_UFS ${.IMPSRC}

