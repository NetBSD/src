#	$NetBSD: Makefile,v 1.27 2000/05/26 03:04:28 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=		ps
SRCS=		fmt.c keyword.c nlist.c print.c ps.c procfs_ops.c
DPADD=		${LIBM} ${LIBKVM}
LDADD=		-lm -lkvm

CWARNFLAGS+=	-Wno-format-y2k

.include <bsd.prog.mk>
