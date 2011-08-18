#	$NetBSD: Makefile,v 1.7 2011/08/18 02:32:32 christos Exp $

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
