#	$NetBSD: Makefile,v 1.3 1997/05/09 07:50:31 mycroft Exp $
#
# DO NOT OPTMIZE bugcrt (i.e. no "-O2")
#
CFLAGS=-I${.CURDIR}/../../include -I${.CURDIR}/../libbug -fomit-frame-pointer

OBJS=bugcrt.o

CLEANFILES+=a.out

all:	${OBJS}

bugcrt.o: bugcrt.c
	${CC} ${CFLAGS} -c ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

proginstall::

.include <bsd.prog.mk>
