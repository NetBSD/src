# $NetBSD: Makefile,v 1.6 1997/09/24 07:55:24 jeremy Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "sun3x") || \
    (${MACHINE} == "atari") || (${MACHINE} == "sparc")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

NOMAN=

.include <bsd.prog.mk>
