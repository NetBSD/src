#	$NetBSD: Makefile,v 1.4 1997/09/05 04:31:55 gwr Exp $

SA_PROG= ufsboot
SRCS= boot.c conf.c version.c
OBJS= ${SRCS:.c=.o}

# Using local rule for this
OBJS+= ufs.o

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

ufs.o : ${S}/lib/libsa/ufs.c
	${COMPILE.c} -DCOMPAT_UFS ${.IMPSRC}

