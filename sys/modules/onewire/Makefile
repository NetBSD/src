#	$NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:35 christos Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/onewire

KMOD=   	onewire
IOCONF=		onewire.ioconf
SRCS=		onewire.c onewire_subr.c onewire_bitbang.c

CPPFLAGS+=	-I${S}/onewire

WARNS=		3

.include <bsd.kmodule.mk>
