#	$NetBSD: Makefile,v 1.1 1998/10/15 15:22:13 ws Exp $

BINDIR=	/usr/mdec
BINMODE=444

STRIPFLAG=

PROG=	mbr
SRCS=	mbr.S
NOMAN=

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp -e _start -N -Ttext 0x600 ${OBJS}
	${STRIPPROG} ${PROG}.tmp
	dd if=${PROG}.tmp of=${PROG} bs=32 skip=1
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
