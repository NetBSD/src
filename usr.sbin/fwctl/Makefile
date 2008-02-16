# $NetBSD: Makefile,v 1.3 2008/02/16 07:27:29 matt Exp $

PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c fwmpegts.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CPPFLAGS+=-I${.CURDIR} -I${SDIR}

.include <bsd.prog.mk>
