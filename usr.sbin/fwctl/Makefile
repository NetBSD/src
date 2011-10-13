# $NetBSD: Makefile,v 1.7 2011/10/13 17:23:29 plunky Exp $

WARNS?=	4
PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c fwmpegts.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CPPFLAGS+=-I${.CURDIR} -I${SDIR}

.include <bsd.prog.mk>
