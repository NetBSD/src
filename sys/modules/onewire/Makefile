#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:56 rin Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/onewire

KMOD=   	onewire
IOCONF=		onewire.ioconf
SRCS=		onewire.c onewire_subr.c onewire_bitbang.c

CPPFLAGS+=	-I${S}/onewire

WARNS=		3

.include <bsd.kmodule.mk>
