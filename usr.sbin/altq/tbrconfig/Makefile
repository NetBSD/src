#	$NetBSD: Makefile,v 1.3 2001/12/14 00:52:34 lukem Exp $

BINDIR=	/sbin
LDSTATIC?=-static

PROG=	tbrconfig
MAN=	tbrconfig.8

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

.include <bsd.prog.mk>
