# $NetBSD: Makefile,v 1.4 2011/01/04 20:45:12 christos Exp $

WARNS?=5
PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c fwmpegts.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CPPFLAGS+=-I${.CURDIR} -I${SDIR}

.include <bsd.prog.mk>
