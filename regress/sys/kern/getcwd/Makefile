#	$NetBSD: Makefile,v 1.1 1999/03/22 18:14:39 sommerfe Exp $

PROG=	getcwd
SRCS=	getcwd.c old_getcwd.c
MKMAN=	no
LDFLAGS=-static
WARNS=1

regress:
	@./getcwd -r

.include <bsd.prog.mk>
