#	$NetBSD: Makefile,v 1.31 2021/09/14 17:09:18 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=		ps
SRCS=		fmt.c keyword.c nlist.c print.c ps.c
DPADD=		${LIBUTIL} ${LIBM} ${LIBKVM}
LDADD=		-lutil -lm -lkvm

COPTS.print.c = -Wno-format-nonliteral -Wno-format-y2k

.include <bsd.prog.mk>
