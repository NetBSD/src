#	$NetBSD: Makefile,v 1.4 2011/08/28 15:48:19 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdafg
IOCONF=	hdafg.ioconf
SRCS=	hdafg.c hdafg_dd.c

.include <bsd.kmodule.mk>
