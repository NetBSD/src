#	$NetBSD: Makefile,v 1.5 2000/07/09 13:47:34 jdolecek Exp $
#
# DO NOT OPTMIZE bugcrt (i.e. no "-O2")
#

S=      ${.CURDIR}/../../../..
PROG= dummy
MAN=

CFLAGS=-I${.CURDIR}/../../include -I${.CURDIR}/../libbug -fomit-frame-pointer \
	-I${S} -I.
SRCS= bugcrt.c
CLEANFILES+= bugcrt.o.tmp

realall: ${OBJS}

bugcrt.o: bugcrt.c
	${CC} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}.tmp
	${LD} -x -r ${.TARGET}.tmp -o ${.TARGET}

${PROG}: ${OBJS}
	@# empty

proginstall::

.include <bsd.prog.mk>
