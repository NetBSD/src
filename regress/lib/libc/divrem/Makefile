# $NetBSD: Makefile,v 1.6 2004/06/30 03:26:27 jmc Exp $

NOMAN=		# defined

PROG=		divremtest
COPTS+=		-Wall -Wno-format -Wno-parentheses -Wno-uninitialized
GOODRESULT=	3f181846a7fd931b9aeafeac6b32d24c

regress: ${PROG}
	[ `./${PROG} | md5` = ${GOODRESULT} ]

.include <bsd.prog.mk>
