#	$NetBSD: Makefile,v 1.1 2003/01/31 20:14:25 skrll Exp $

WARNS=1

PROG=   preempt1
SRCS=   preempt1.c

LDADD=	-lpthread

NOMAN=

regress:
	@echo Testing kernel preemption during a large uiomove.
	./preempt1

.include <bsd.prog.mk>
