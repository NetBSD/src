#	$NetBSD: Makefile,v 1.19 2003/02/26 15:01:09 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	w
SRCS=	fmt.c pr_time.c proc_compare.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
LINKS=	${BINDIR}/w ${BINDIR}/uptime
CPPFLAGS+= -DSUPPORT_UTMP -DSUPPORT_UTMPX

.PATH: ${NETBSDSRCDIR}/bin/ps

.include <bsd.prog.mk>
