# $NetBSD: Makefile,v 1.7 1997/12/17 21:16:54 scw Exp $
# edlabel (Edit Disk LABEL)

.if (${MACHINE} == "sun3") || (${MACHINE} == "sun3x") || \
    (${MACHINE} == "atari") || (${MACHINE} == "sparc") || \
	(${MACHINE} == "mvme68k")
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

NOMAN=

.include <bsd.prog.mk>
