#	$NetBSD: Makefile,v 1.4 2002/08/27 14:46:19 lukem Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	tbrconfig
MAN=	tbrconfig.8

BINDIR=	/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

.include <bsd.prog.mk>
