# $NetBSD: Makefile,v 1.18 2009/12/13 05:01:32 nakayama Exp $
# edlabel (Edit Disk LABEL)

NOMAN=	# defined

.if ${MACHINE_ARCH} == "m68000" || ${MACHINE_ARCH} == "m68k" || \
    ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

.include <bsd.prog.mk>
