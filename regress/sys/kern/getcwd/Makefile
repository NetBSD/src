#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:53 lukem Exp $

NOMAN=		# defined
LDSTATIC=	-static

PROG=		getcwd
SRCS=		getcwd.c old_getcwd.c
WARNS=		1

regress:
	@./getcwd -r
	@./getcwd -p

long-regress:
	@find /usr -type d -print | ./getcwd -s

.include <bsd.prog.mk>
