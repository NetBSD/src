#	Makefile,v 1.3 2007/05/28 12:06:38 tls Exp

USE_FORT?= yes	# network server

PROG=	ndbootd
SRCS=	ndbootd.c
MAN=	ndbootd.8

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}

.include <bsd.own.mk>
.include <bsd.prog.mk>
