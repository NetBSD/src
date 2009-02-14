# $NetBSD: Makefile,v 1.15 2009/02/14 11:33:56 abs Exp $
# edlabel (Edit Disk LABEL)

NOMAN=	# defined

.if ${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE_ARCH} == "sparc64"
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

.include <bsd.prog.mk>
