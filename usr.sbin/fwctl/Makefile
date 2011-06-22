# $NetBSD: Makefile,v 1.5 2011/06/22 02:49:45 mrg Exp $

WARNS?=5
PROG=	fwctl
SRCS=	eui64.c fwcontrol.c fwcrom.c fwdv.c fwmpegts.c
MAN=	fwctl.8

.PATH:	${.CURDIR}/../../sys/dev/ieee1394

SDIR= ${.CURDIR}/../../sys
CPPFLAGS+=-I${.CURDIR} -I${SDIR}

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.fwcontrol.c+=	-fno-strict-aliasing
.endif
