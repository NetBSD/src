#	$NetBSD: Makefile,v 1.4 2000/06/25 16:39:07 scw Exp $
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
