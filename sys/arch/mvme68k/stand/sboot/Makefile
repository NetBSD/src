#	$NetBSD: Makefile,v 1.13 2002/04/09 19:37:22 scw Exp $

#
# sboot would like a newer GNU ld because it can generate S-Records.
# Until then, we convert.
#

COMPILE.s= $(CC) -x assembler-with-cpp -traditional-cpp \
	$(ASFLAGS) $(CPPFLAGS) $(INCPATH) -c -o $*.o

RELOC=		0x4000
SRCS=		start.s clock.c console.c etherfun.c le_poll.c \
		oc_cksum.s sboot.c
SRTOBJ=
PROG=		sboot
LIBS=		${LIBSA}
DPADD=		${LIBS}
CLEANFILES+=	sboot.srec

PROGDEPENDS=	${OBJS} ${LIBS}
PROGLINKOBJS=	${OBJS} ${LIBS}

sboot.srec:	${PROG}
	${OBJCOPY} -O srec ${.OBJDIR}/sboot ${.TARGET}

.include "../Makefile.booters"

all realall:	sboot.srec

install:	sboot.srec
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/sboot.srec ${DESTDIR}${MDEC_DIR}/sboot

.include <bsd.prog.mk>
