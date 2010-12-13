#	$NetBSD: Makefile,v 1.17 2010/12/13 19:19:10 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

SRCDIR=		${.CURDIR}/../../../sbin/route

PROG=		route
NOMAN=		# defined

.include <bsd.own.mk>

SRCS=		route.c show.c keywords.c

CPPFLAGS+=	-DSMALL -I${SRCDIR} -DCRUNCHOPS
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

BINOWN=		root
BINMODE=	4555
.PATH:		${SRCDIR}
