# $NetBSD: Makefile,v 1.11 2001/12/12 01:48:43 tv Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "sun3x") || \
    (${MACHINE} == "atari") || (${MACHINE} == "sparc") || \
    (${MACHINE} == "sparc64") || (${MACHINE} == "mvme68k") || \
    (${MACHINE} == "sun2")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

NOMAN=	# defined

.include <bsd.prog.mk>
