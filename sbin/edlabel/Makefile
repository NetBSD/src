# $NetBSD: Makefile,v 1.5 1997/09/14 13:26:11 lukem Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "atari") || \
    (${MACHINE} == "sparc")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

NOMAN=

.include <bsd.prog.mk>
