#	$NetBSD: Makefile,v 1.9 2000/11/30 22:26:27 scw Exp $

#
# sboot would like a newer GNU ld because it can generate S-Records.
# Until then, we convert.
#

COMPILE.s= $(CC) -x assembler-with-cpp -traditional-cpp \
	$(ASFLAGS) $(CPPFLAGS) -c -o $*.o
LDFLAGS=-x -N -Ttext 0x4000 -e start
SRCS=	start.s clock.c console.c etherfun.c le_poll.c oc_cksum.s sboot.c
CLEANFILES+= srec sboot
# OPTS= -Os

PROG=		sboot.srec
PROGNAME=	sboot
DPADD=	srec sboot

${PROG}:
	${.OBJDIR}/srec 4 0x4000 ${.OBJDIR}/sboot > ${.TARGET} < ${.OBJDIR}/sboot

sboot: ${OBJS}
	${LD} ${LDFLAGS} -o ${.TARGET} ${OBJS} ${LIBSA}
	${OBJCOPY} -O binary ${.OBJDIR}/sboot

srec: srec.c
	${HOST_CC} ${COPTS} ${.IMPSRC} -o ${.TARGET}

all: ${OBJS} ${PROG}

.include "../Makefile.booters"

.include <bsd.prog.mk>
