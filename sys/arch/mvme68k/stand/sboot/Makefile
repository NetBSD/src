#	$NetBSD: Makefile,v 1.8 2000/07/24 18:40:00 jdolecek Exp $

#
# sboot would like a newer GNU ld because it can generate S-Records.
# Until then, we convert.
#

COMPILE.s= $(AS) $(ASFLAGS) -o $*.o
LDFLAGS=-x -N -Ttext 0x4000 -e start
SRCS=	start.s clock.c console.c etherfun.c le_poll.c oc_cksum.s sboot.c
CLEANFILES+= srec sboot
# OPTS= -Os

PROG=		sboot.srec
PROGNAME=	sboot
DPADD=	srec sboot

${PROG}:
	dd ibs=32 skip=1 if=${.OBJDIR}/sboot | ${.OBJDIR}/srec 4 0x4000 ${.OBJDIR}/sboot > ${.TARGET}

sboot: ${OBJS}
	${LD} ${LDFLAGS} -o ${.TARGET} ${OBJS} ${LIBSA}

srec: srec.c
	${CC} ${COPTS} ${.IMPSRC} -o ${.TARGET}

all: ${OBJS} ${PROG}

.include "../Makefile.booters"

.include <bsd.prog.mk>
