#	$NetBSD: Makefile,v 1.1 1996/05/17 19:26:43 chuck Exp $
#
# DO NOT OPTMIZE THIS FILE!!!
#
CFLAGS=-I${.CURDIR}/../../include -fomit-frame-pointer

OBJS=bugcrt.o

CLEANFILES+=a.out

all:	${OBJS}

bugcrt.o: bugcrt.c
	${CC} ${CFLAGS} -c ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

install:

lint tags:

.include <bsd.prog.mk>
