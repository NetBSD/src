#	$NetBSD: Makefile,v 1.6 2002/09/18 05:41:50 lukem Exp $

NOMAN=		# defined

PROG=		emuspeed
SRCS=		emuspeed.c test.S mul64.S div64.S # movepto.c movepfrom.c

.include <bsd.prog.mk>

regress: all
	@./emuspeed
