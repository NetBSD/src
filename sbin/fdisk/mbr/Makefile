#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:33 lukem Exp $

BINDIR=	/usr/mdec
BINMODE=444

STRIPFLAG=

PROG=	mbr
SRCS=	mbr.S
MKMAN=	no

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp -e _start -N -Ttext 0x600 ${OBJS}
	${STRIPPROG} ${PROG}.tmp
	dd if=${PROG}.tmp of=${PROG} bs=32 skip=1
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
