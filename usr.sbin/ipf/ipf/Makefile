#	$NetBSD: Makefile,v 1.12 2000/05/03 11:40:54 veego Exp $

PROG=	ipf
SRCS=	ipf.c parse.c common.c opt.c facpri.c
MAN=	ipf.8 ipf.4 ipf.5 ipl.4
MLINKS=	ipl.4 ipfilter.4 ipf.5 ipf.conf.5

BINDIR=		/sbin
LDSTATIC?=	-static

.include <bsd.prog.mk>
