#	$NetBSD: Makefile,v 1.3.6.1 1999/12/27 18:30:19 wrstuden Exp $
#
PROG=emuspeed
MKMAN=no

SRCS=emuspeed.c test.S mul64.S div64.S # movepto.c movepfrom.c

/* don't install: */
proginstall:

.include <bsd.prog.mk>

regress: all
	@./emuspeed
