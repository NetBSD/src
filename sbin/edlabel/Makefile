# $NetBSD: Makefile,v 1.4 1997/06/23 04:23:49 mrg Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "atari") || \
    (${MACHINE} == "sparc")
PROG=	edlabel
CFLAGS += -Wall
LDADD  += -lutil
DPADD  += ${LIBUTIL}

all: ${PROG}
.endif

NOMAN=

.include <bsd.prog.mk>
