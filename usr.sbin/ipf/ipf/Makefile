#	$NetBSD: Makefile,v 1.10 1998/03/02 19:48:50 cgd Exp $

.include <bsd.own.mk>

PROG=	ipf
SRCS=	ipf.c parse.c opt.c
MAN=	ipf.8 ipf.4 ipf.5 ipl.4
MLINKS=	ipl.4 ipfilter.4 ipf.5 ipf.conf.5

BINDIR=		/sbin
LDSTATIC?=	-static

.include <bsd.prog.mk>
