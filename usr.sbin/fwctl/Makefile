# $NetBSD: Makefile,v 1.7.54.1 2025/08/02 05:58:49 perseant Exp $

WARNS?=	4
PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c fwmpegts.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CPPFLAGS+=-I${.CURDIR} -I${SDIR}
LINTFLAGS+= -X 34	# unportable bit-field type 'unsigned char'

.include <bsd.prog.mk>
