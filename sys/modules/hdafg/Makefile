#	$NetBSD: Makefile,v 1.3.2.2 2011/03/05 20:55:42 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdafg
IOCONF=	hdafg.ioconf
SRCS=	hdafg.c hdafg_dd.c

WARNS=	3

.include <bsd.kmodule.mk>
