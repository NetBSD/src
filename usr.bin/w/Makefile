#	$NetBSD: Makefile,v 1.16.2.1 2002/11/12 20:32:03 nathanw Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	w
SRCS=	fmt.c pr_time.c proc_compare.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
LINKS=	${BINDIR}/w ${BINDIR}/uptime
CPPFLAGS+= -DSUPPORT_FTPD_UTMP -DSUPPORT_UTMP -DSUPPORT_UTMPX

.PATH: ${NETBSDSRCDIR}/bin/ps

.include <bsd.prog.mk>
