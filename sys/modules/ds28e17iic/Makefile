#	$NetBSD: Makefile,v 1.1 2025/01/23 19:02:42 brad Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/onewire

KMOD=   	ds28e17iic
IOCONF=		ds28e17iic.ioconf
SRCS=		ds28e17iic.c

CPPFLAGS+=	-I${S}/onewire

WARNS=		3

.include <bsd.kmodule.mk>
