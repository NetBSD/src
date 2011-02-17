#	$NetBSD: Makefile,v 1.1.4.1 2011/02/17 12:00:48 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdafg
IOCONF=	hdafg.ioconf
SRCS=	hdafg.c hdafg_dd.c

WARNS=	3

.include <bsd.kmodule.mk>
