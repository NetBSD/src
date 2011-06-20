#	$NetBSD: Makefile,v 1.4 2011/06/20 07:44:02 mrg Exp $

PROG	 =	isdntrace
SRCS	 =	q921.c q931.c q931_util.c q932_fac.c 1tr6.c trace.c \
		pcause_1tr6.c pcause_q850.c unknownl3.c
MAN      =	isdntrace.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.q932_fac.c+=		-Wno-pointer-sign
COPTS.trace.c+=			-Wno-pointer-sign
.endif
