#	$NetBSD: Makefile,v 1.10 2020/05/17 23:34:11 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	cp
SRCS=	cp.c utils.c

.ifdef SMALLPROG
CPPFLAGS+=-DSMALL
.endif

.include <bsd.prog.mk>
