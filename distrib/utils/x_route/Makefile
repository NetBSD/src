#	$NetBSD: Makefile,v 1.16 2005/01/18 12:30:21 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

SRCDIR=		${.CURDIR}/../../../sbin/route

PROG=		route
NOMAN=		# defined

.include <bsd.own.mk>

SRCS=		route.c show.c keywords.c

CPPFLAGS+=	-DSMALL -I${SRCDIR}
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

BINOWN=		root
BINMODE=	4555
.PATH:		${SRCDIR}
