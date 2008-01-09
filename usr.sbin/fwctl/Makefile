# $NetBSD: Makefile,v 1.1.10.1 2008/01/09 02:02:01 matt Exp $

PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c fwmpegts.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CFLAGS+=-I${.CURDIR} -I${SDIR}

.include <bsd.prog.mk>
