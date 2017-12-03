#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:19 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tun
COMMENT=/dev/tun network tunneling over tty

IOCONF=	TUN.ioconf
SRCS=	if_tun.c

SRCS+=	tun_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
