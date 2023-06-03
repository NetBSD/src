#	$NetBSD: Makefile,v 1.23 2023/06/03 09:09:19 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	w
SRCS=	fmt.c pr_time.c w.c
MAN=	w.1 uptime.1
DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil
LINKS=	${BINDIR}/w ${BINDIR}/uptime
CPPFLAGS+= -DSUPPORT_UTMP -DSUPPORT_UTMPX

.PATH: ${NETBSDSRCDIR}/bin/ps

COPTS.pr_time.c += -Wno-format-y2k
COPTS.w.c += ${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
