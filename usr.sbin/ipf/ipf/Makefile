#	$NetBSD: Makefile,v 1.11 1999/12/12 11:29:58 veego Exp $

PROG=	ipf
SRCS=	ipf.c parse.c facpri.c opt.c
MAN=	ipf.8 ipf.4 ipf.5 ipl.4
MLINKS=	ipl.4 ipfilter.4 ipf.5 ipf.conf.5

BINDIR=		/sbin
LDSTATIC?=	-static

.include <bsd.prog.mk>
