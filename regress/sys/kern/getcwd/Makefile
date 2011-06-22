#	$NetBSD: Makefile,v 1.6 2011/06/22 02:49:43 mrg Exp $

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

# getcwd.c has an on-purpuse array bounds violation
.if ${HAVE_GCC} == 45
COPTS.getcwd.c+= -Wno-error
.endif
