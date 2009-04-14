#	$NetBSD: Makefile,v 1.7 2009/04/14 22:15:27 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

CPPFLAGS+=	-DNOT_CSH
CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/bin
SRCS=	time.c strpct.c xtime.c
PROG=	time

.PATH: ${NETBSDSRCDIR}/bin/csh

.include <bsd.prog.mk>
