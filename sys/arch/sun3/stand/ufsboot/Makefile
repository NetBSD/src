#	$NetBSD: Makefile,v 1.3.12.1 1997/09/06 18:44:24 thorpej Exp $

SA_PROG= ufsboot
SRCS= boot.c conf.c version.c
OBJS= ${SRCS:.c=.o}

# Using local rule for this
OBJS+= ufs.o

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

ufs.o : ${S}/lib/libsa/ufs.c
	${COMPILE.c} -DCOMPAT_UFS ${.IMPSRC}

