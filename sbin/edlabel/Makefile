# $NetBSD: Makefile,v 1.8.10.1 2000/08/26 00:45:00 mrg Exp $
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
