#	$NetBSD: Makefile,v 1.5.8.2 2000/12/08 09:28:44 bouyer Exp $

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
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.OBJDIR}/sboot.srec \
	   ${DESTDIR}${MDECDIR}/sboot

.include <bsd.prog.mk>
