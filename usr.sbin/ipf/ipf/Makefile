#	$NetBSD: Makefile,v 1.13 2002/01/24 08:24:38 martti Exp $

PROG=	ipf
SRCS=	ipf.c parse.c common.c opt.c facpri.c
MAN=	ipf.8 ipf.4 ipf.5 ipl.4
MLINKS=	ipl.4 ipfilter.4 ipf.5 ipf.conf.5 ipf6.conf.5

BINDIR=		/sbin
LDSTATIC?=	-static

.include <bsd.prog.mk>
