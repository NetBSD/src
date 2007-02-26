#	$NetBSD: Makefile,v 1.6 2007/02/26 00:17:37 matt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

WARNS=2
CPPFLAGS+=	-DNOT_CSH
CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/bin
SRCS=	time.c strpct.c xtime.c
PROG=	time

.PATH: ${NETBSDSRCDIR}/bin/csh

.include <bsd.prog.mk>
