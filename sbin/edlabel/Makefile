# $NetBSD: Makefile,v 1.8 1999/02/13 02:54:33 lukem Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "sun3x") || \
    (${MACHINE} == "atari") || (${MACHINE} == "sparc") || \
	(${MACHINE} == "mvme68k")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

MKMAN=	no

.include <bsd.prog.mk>
