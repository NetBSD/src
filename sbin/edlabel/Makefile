# $NetBSD: Makefile,v 1.14.36.1 2009/05/13 19:19:00 jym Exp $
# edlabel (Edit Disk LABEL)

NOMAN=	# defined

.if ${MACHINE_ARCH} == "m68000" || ${MACHINE_ARCH} == "m68k" || \
    ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
PROG=	edlabel
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

.include <bsd.prog.mk>
