#	$NetBSD: Makefile,v 1.5 2007/02/24 21:30:27 matt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS=2
CPPFLAGS+=	-DNOT_CSH
CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/bin
SRCS=	time.c strpct.c xtime.c
PROG=	time

.PATH: ${NETBSDSRCDIR}/bin/csh

.include <bsd.prog.mk>
