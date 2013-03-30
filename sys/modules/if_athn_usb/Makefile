# $NetBSD: Makefile,v 1.1 2013/03/30 18:59:38 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb
.PATH:	${S}/dev/ic

KMOD=	if_athn_usb
IOCONF=	if_athn_usb.ioconf
SRCS=	if_athn_usb.c
SRCS+=	athn.c
SRCS+=	arn5008.c
SRCS+=	arn5416.c
SRCS+=	arn9003.c
SRCS+=	arn9280.c
SRCS+=	arn9285.c
SRCS+=	arn9287.c
SRCS+=	arn9380.c

WARNS=	4

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DNATHN_USB=1
CPPFLAGS+=	-DATHN_DEBUG

.include <bsd.kmodule.mk>
