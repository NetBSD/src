# $NetBSD: Makefile,v 1.9 2000/08/09 11:37:34 mrg Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "sun3x") || \
    (${MACHINE} == "atari") || (${MACHINE} == "sparc") || \
    (${MACHINE} == "sparc64") || (${MACHINE} == "mvme68k")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

MKMAN=	no

.include <bsd.prog.mk>
