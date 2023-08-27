#	$NetBSD: Makefile,v 1.1 2023/08/27 13:20:09 kardel Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/onewire

KMOD=   	owtemp
IOCONF=		owtemp.ioconf
SRCS=		owtemp.c

CPPFLAGS+=	-I${S}/onewire

WARNS=		3

.include <bsd.kmodule.mk>
