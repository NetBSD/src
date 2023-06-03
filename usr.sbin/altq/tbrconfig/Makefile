#	$NetBSD: Makefile,v 1.6 2023/06/03 09:09:20 lukem Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	tbrconfig
MAN=	tbrconfig.8

BINDIR=	/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

COPTS.tbrconfig.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
