#	$NetBSD: Makefile,v 1.10.2.1 1999/12/20 21:06:48 he Exp $

PROG=	ipf
SRCS=	ipf.c parse.c facpri.c opt.c
MAN=	ipf.8 ipf.4 ipf.5 ipl.4
MLINKS=	ipl.4 ipfilter.4 ipf.5 ipf.conf.5

BINDIR=		/sbin
LDSTATIC?=	-static

.include <bsd.prog.mk>
