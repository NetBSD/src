#	$NetBSD: Makefile,v 1.5 2001/12/12 01:24:14 tv Exp $

PROG=		emuspeed
NOMAN=		# defined
SRCS=		emuspeed.c test.S mul64.S div64.S # movepto.c movepfrom.c

.include <bsd.prog.mk>

regress: all
	@./emuspeed
