#	$NetBSD: Makefile,v 1.1 2011/08/31 12:46:36 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/onewire

KMOD=   	onewire
IOCONF=		onewire.ioconf
SRCS=		onewire.c onewire_subr.c onewire_bitbang.c

CPPFLAGS+=	-I${S}/onewire

.include <bsd.kmodule.mk>
