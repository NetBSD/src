# $NetBSD: Makefile,v 1.12 2002/09/18 06:38:50 lukem Exp $
# edlabel (Edit Disk LABEL)

NOMAN=	# defined

.if (${MACHINE} == "sun3") || (${MACHINE} == "sun3x") || \
    (${MACHINE} == "atari") || (${MACHINE} == "sparc") || \
    (${MACHINE} == "sparc64") || (${MACHINE} == "mvme68k") || \
    (${MACHINE} == "sun2")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

.include <bsd.prog.mk>
