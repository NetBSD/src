# $NetBSD: Makefile,v 1.17 2009/02/16 13:38:21 is Exp $
# edlabel (Edit Disk LABEL)

NOMAN=	# defined

.if ${MACHINE_ARCH} == "m68000" || ${MACHINE_ARCH} == "m68k" || \
    ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

.include <bsd.prog.mk>
