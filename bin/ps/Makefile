#	$NetBSD: Makefile,v 1.30 2021/04/06 04:49:41 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=		ps
SRCS=		fmt.c keyword.c nlist.c print.c ps.c
DPADD=		${LIBM} ${LIBKVM}
LDADD=		-lm -lkvm

COPTS.print.c = -Wno-format-nonliteral -Wno-format-y2k

.include <bsd.prog.mk>
