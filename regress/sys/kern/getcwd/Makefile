#	$NetBSD: Makefile,v 1.2 1999/03/26 13:14:12 sommerfe Exp $

PROG=	getcwd
SRCS=	getcwd.c old_getcwd.c
MKMAN=	no
LDFLAGS=-static
WARNS=1

regress:
	@./getcwd -r
	@./getcwd -p

long-regress:
	@find /usr -type d -print | ./getcwd -s

.include <bsd.prog.mk>
