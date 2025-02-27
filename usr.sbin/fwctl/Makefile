# $NetBSD: Makefile,v 1.8 2025/02/27 21:09:23 rillig Exp $

WARNS?=	4
PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c fwmpegts.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CPPFLAGS+=-I${.CURDIR} -I${SDIR}
LINTFLAGS+= -X 34	# unportable bit-field type 'unsigned char'

.include <bsd.prog.mk>
