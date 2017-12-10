#	$NetBSD: Makefile,v 1.2 2017/12/10 20:38:14 bouyer Exp $
#

.include <bsd.own.mk>

RUMPTOP= ${NETBSDSRCDIR}/sys/rump
.PATH:	${.CURDIR}/../../../../sys/dev/hid

LIB=	rumpdev_hid
LIBISPRIVATE= #defined

SRCS=	hid.c

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
