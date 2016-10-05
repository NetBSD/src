#	$NetBSD: Makefile,v 1.1.4.2 2016/10/05 20:56:11 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tun
COMMENT=/dev/tun network tunneling over tty

IOCONF=	TUN.ioconf
SRCS=	if_tun.c

SRCS+=	tun_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
