#	$NetBSD: Makefile,v 1.5 1998/02/05 04:57:23 gwr Exp $

SA_PROG= ufsboot
SRCS= boot.c conf.c
OBJS= ${SRCS:.c=.o}

# Using local rule for this
OBJS+= ufs.o

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

ufs.o : ${S}/lib/libsa/ufs.c
	${COMPILE.c} -DCOMPAT_UFS ${.IMPSRC}

