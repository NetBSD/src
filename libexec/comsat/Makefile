#	$NetBSD: Makefile,v 1.7 2009/03/16 02:24:56 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

PROG=	comsat
SRCS=	comsat.c utmpentry.c
MAN=	comsat.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
