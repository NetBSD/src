#	$NetBSD: Makefile,v 1.15 2002/08/27 14:46:21 lukem Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	ipf
SRCS=	ipf.c parse.c common.c opt.c facpri.c
MAN=	ipf.8 ipf.4 ipf.5 ipl.4
MLINKS=	ipl.4 ipfilter.4 ipf.5 ipf.conf.5 ipf.5 ipf6.conf.5

BINDIR=		/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.include <bsd.prog.mk>
