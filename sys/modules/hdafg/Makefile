#	$NetBSD: Makefile,v 1.3 2011/02/12 15:15:56 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdafg
IOCONF=	hdafg.ioconf
SRCS=	hdafg.c hdafg_dd.c

WARNS=	3

.include <bsd.kmodule.mk>
