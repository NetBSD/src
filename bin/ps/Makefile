#	$NetBSD: Makefile,v 1.29.44.2 2020/04/21 19:37:33 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=		ps
SRCS=		fmt.c keyword.c nlist.c print.c ps.c
DPADD=		${LIBM} ${LIBKVM}
LDADD=		-lm -lkvm

CWARNFLAGS+=	-Wno-format-y2k
COPTS.print.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
