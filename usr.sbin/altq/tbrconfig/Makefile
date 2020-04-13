#	$NetBSD: Makefile,v 1.4.98.1 2020/04/13 08:05:51 martin Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	tbrconfig
MAN=	tbrconfig.8

BINDIR=	/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

COPTS.tbrconfig.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
