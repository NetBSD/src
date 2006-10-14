#	$NetBSD: Makefile,v 1.5 2006/10/14 21:23:01 bjh21 Exp $

NOMAN=		# defined
LDSTATIC=	-static

PROG=		getcwd
SRCS=		getcwd.c old_getcwd.c
WARNS=		1

regress: ${PROG}
	@./getcwd -r
	@./getcwd -p

long-regress:
	@find /usr -type d -print | ./getcwd -s

.include <bsd.prog.mk>
