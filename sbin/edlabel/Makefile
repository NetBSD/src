# $NetBSD: Makefile,v 1.14 2005/01/20 16:39:23 xtraeme Exp $
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
