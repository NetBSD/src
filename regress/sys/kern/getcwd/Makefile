#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:16 tv Exp $

PROG=		getcwd
SRCS=		getcwd.c old_getcwd.c
NOMAN=		# defined
LDSTATIC=	-static
WARNS=		1

regress:
	@./getcwd -r
	@./getcwd -p

long-regress:
	@find /usr -type d -print | ./getcwd -s

.include <bsd.prog.mk>
