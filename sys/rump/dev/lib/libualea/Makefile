#	$NetBSD: Makefile,v 1.1 2017/04/17 08:59:37 riastradh Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_ualea
COMMENT=USB Araneus Alea II TRNG

IOCONF=	UALEA.ioconf

SRCS+=	ualea.c
SRCS+=	ualea_component.c


.include <bsd.lib.mk>
.include <bsd.klinks.mk>
