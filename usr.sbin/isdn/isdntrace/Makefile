#	$NetBSD: Makefile,v 1.5 2012/08/10 12:10:28 joerg Exp $

PROG	 =	isdntrace
SRCS	 =	q921.c q931.c q931_util.c q932_fac.c 1tr6.c trace.c \
		pcause_1tr6.c pcause_q850.c unknownl3.c
MAN      =	isdntrace.8

.include <bsd.prog.mk>

COPTS.q932_fac.c+=		-Wno-pointer-sign
COPTS.trace.c+=			-Wno-pointer-sign
