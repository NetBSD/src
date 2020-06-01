#	$NetBSD: Makefile,v 1.3 2020/06/01 14:41:25 christos Exp $
#

.include <bsd.own.mk>

RUMPTOP= ${NETBSDSRCDIR}/sys/rump
.PATH:	${.CURDIR}/../../../../sys/dev/hid

LIB=	rumpdev_hid
LIBISPRIVATE= yes

SRCS=	hid.c

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
