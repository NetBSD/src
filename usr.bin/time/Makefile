#	$NetBSD: Makefile,v 1.6.20.1 2009/05/13 19:20:08 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

CPPFLAGS+=	-DNOT_CSH
CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/bin
SRCS=	time.c strpct.c xtime.c
PROG=	time

.PATH: ${NETBSDSRCDIR}/bin/csh

.include <bsd.prog.mk>
