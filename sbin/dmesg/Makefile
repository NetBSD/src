#	$NetBSD: Makefile,v 1.12 2005/09/05 20:56:41 dsl Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	dmesg
MAN=	dmesg.8
LDADD=	-lkvm
DPADD=	${LIBKVM}

.ifdef SMALLPROG
CPPFLAGS+=-DSMALL
.endif

.include <bsd.prog.mk>
