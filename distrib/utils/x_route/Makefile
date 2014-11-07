#	$NetBSD: Makefile,v 1.18 2014/11/07 02:57:19 uebayasi Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

SRCDIR=		${.CURDIR}/../../../sbin/route

PROG=		route
NOMAN=		# defined

.include <bsd.own.mk>

SRCS=		keywords.c route.c rtutil.c show.c

CPPFLAGS+=	-DSMALL -I${SRCDIR} -DCRUNCHOPS
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

BINOWN=		root
BINMODE=	4555
.PATH:		${SRCDIR}
