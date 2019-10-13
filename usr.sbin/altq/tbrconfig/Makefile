#	$NetBSD: Makefile,v 1.5 2019/10/13 07:28:17 mrg Exp $

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
