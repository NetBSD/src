#	$NetBSD: Makefile,v 1.2 1996/09/07 19:23:03 thorpej Exp $

#
# sboot would like a newer GNU ld because it can generate S-Records.
# Until then, we convert.
#

COMPILE.s= $(AS) $(ASFLAGS) -o $*.o
LDFLAGS=-x -N -Ttext 0x4000 -e start
NOMAN=
SRCS=	start.s clock.c console.c etherfun.c le_poll.c libc_sa.c \
		oc_cksum.s sboot.c
OBJS=	${SRCS:N*.h:R:S/$/.o/g}

BOOTS=	sboot.srec

all: ${BOOTS}

sboot.srec: sboot srec
	dd ibs=32 skip=1 if=sboot | ${.OBJDIR}/srec 4 0x4000 sboot > ${.TARGET}

sboot: ${OBJS}
	${LD} ${LDFLAGS} -o ${.TARGET} ${OBJS}

srec: ${.CURDIR}/srec.c
	${CC} ${CFLAGS} ${.CURDIR}/srec.c -o ${.TARGET}

install:
	install -c -m 444 -g bin -o bin sboot.srec ${DESTDIR}${MDEC_DIR}/sboot

.include <bsd.prog.mk>
