#	$NetBSD: Makefile,v 1.17 2002/07/27 23:58:40 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	w
SRCS=	fmt.c pr_time.c proc_compare.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
LINKS=	${BINDIR}/w ${BINDIR}/uptime
CPPFLAGS+= -DSUPPORT_FTPD_UTMP -DSUPPORT_UTMP -DSUPPORT_UTMPX

.PATH: ${.CURDIR}/../../bin/ps

.include <bsd.prog.mk>
