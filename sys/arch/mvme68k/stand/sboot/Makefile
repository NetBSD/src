#	$NetBSD: Makefile,v 1.11.4.1 2002/03/16 15:58:58 jdolecek Exp $

#
# sboot would like a newer GNU ld because it can generate S-Records.
# Until then, we convert.
#

COMPILE.s= $(CC) -x assembler-with-cpp -traditional-cpp \
	$(ASFLAGS) $(CPPFLAGS) $(INCPATH) -c -o $*.o

RELOC=	0x4000
SRCS=	start.s clock.c console.c etherfun.c le_poll.c oc_cksum.s sboot.c
SRTOBJ=
PROG=		sboot
PROGNAME=	sboot.srec
LIBS=		${LIBSA}
DPADD=		${LIBS}
CLEANFILES+=	srec sboot.srec

PROGDEPENDS=	${OBJS} ${LIBS}
PROGLINKOBJS=	${OBJS} ${LIBS}

sboot.srec:	${PROG} srec
	${OBJCOPY} -O binary ${.OBJDIR}/sboot
	${.OBJDIR}/srec 4 0x4000 ${.OBJDIR}/sboot \
	> ${.TARGET} < ${.OBJDIR}/sboot

srec: srec.c
	${HOST_CC} ${COPTS} ${.IMPSRC} -o ${.TARGET}

.include "../Makefile.booters"

all realall:	sboot.srec

install:	sboot.srec
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/sboot.srec ${DESTDIR}${MDEC_DIR}/sboot

.include <bsd.prog.mk>
