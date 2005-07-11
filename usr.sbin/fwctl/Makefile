# $NetBSD: Makefile,v 1.1 2005/07/11 15:35:25 kiyohara Exp $

PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CFLAGS+=-I${.CURDIR} -I${SDIR}

.include <bsd.prog.mk>
