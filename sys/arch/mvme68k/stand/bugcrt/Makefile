#	$NetBSD: Makefile,v 1.3.36.1 2000/06/26 18:14:50 scw Exp $
#
# DO NOT OPTMIZE bugcrt (i.e. no "-O2")
#
CFLAGS=-I${.CURDIR}/../../include -I${.CURDIR}/../libbug -fomit-frame-pointer

OBJS=bugcrt.o

CLEANFILES+=a.out

realall:	${OBJS}

bugcrt.o: bugcrt.c
	${CC} ${CFLAGS} -c ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

proginstall::

.include <bsd.prog.mk>
