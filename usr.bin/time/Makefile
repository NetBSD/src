#	$NetBSD: Makefile,v 1.8 2011/08/17 13:29:39 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

CPPFLAGS+=	-DNOT_CSH
CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/bin
SRCS=	time.c strpct.c xtime.c
PROG=	time

COPTS.strpct.c += -Wno-format-nonliteral

.PATH: ${NETBSDSRCDIR}/bin/csh

.include <bsd.prog.mk>
