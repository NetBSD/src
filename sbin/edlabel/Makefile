# $NetBSD: Makefile,v 1.10 2001/06/13 18:09:15 fredette Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "sun3x") || \
    (${MACHINE} == "atari") || (${MACHINE} == "sparc") || \
    (${MACHINE} == "sparc64") || (${MACHINE} == "mvme68k") || \
    (${MACHINE} == "sun2")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

MKMAN=	no

.include <bsd.prog.mk>
