#	$NetBSD: Makefile,v 1.15 2000/05/26 03:10:31 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	w
SRCS=	fmt.c pr_time.c proc_compare.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
LINKS=	${BINDIR}/w ${BINDIR}/uptime

.PATH: ${.CURDIR}/../../bin/ps

.include <bsd.prog.mk>
