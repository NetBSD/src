#	$NetBSD: Makefile,v 1.4 1999/12/09 09:08:00 itohy Exp $
#
PROG=emuspeed
MKMAN=no

SRCS=emuspeed.c test.S mul64.S div64.S # movepto.c movepfrom.c

/* don't install: */
proginstall:

.include <bsd.prog.mk>

regress: all
	@./emuspeed
