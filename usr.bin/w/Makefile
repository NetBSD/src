#	$NetBSD: Makefile,v 1.15.4.1 2000/09/28 18:48:50 jdolecek Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	w
SRCS=	fmt.c pr_time.c proc_compare.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
LINKS=	${BINDIR}/w ${BINDIR}/uptime
CPPFLAGS+= -DSUPPORT_FTPD_UTMP

.PATH: ${.CURDIR}/../../bin/ps

.include <bsd.prog.mk>
