#	$NetBSD: Makefile,v 1.15 2002/10/17 02:05:40 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

SRCDIR=		${.CURDIR}/../../../sbin/route

PROG=		route
NOMAN=		# defined

SRCS=		route.c show.c keywords.c

CPPFLAGS+=	-DSMALL -I${SRCDIR}
.if defined(SMALLPROG_INET6)
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

BINOWN=		root
BINMODE=	4555
.PATH:		${SRCDIR}
