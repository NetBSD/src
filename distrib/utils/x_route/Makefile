#	$NetBSD: Makefile,v 1.13.2.1 2002/11/03 13:19:41 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

SRCDIR=		${.CURDIR}/../../../sbin/route

PROG=		route
NOMAN=		# defined

SRCS=		route.c show.c keywords.c

CPPFLAGS+=	-DSMALL -I${SRCDIR}
.if (${MACHINE} == "i386" || ${MACHINE} == "hpcmips" || ${MACHINE} == "x68k" || ${MACHINE} == "macppc")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

BINOWN=		root
BINMODE=	4555
.PATH:		${SRCDIR}
