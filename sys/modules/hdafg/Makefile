#	$NetBSD: Makefile,v 1.1.2.1 2011/06/06 09:09:48 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdafg
IOCONF=	hdafg.ioconf
SRCS=	hdafg.c hdafg_dd.c

WARNS=	3

.include <bsd.kmodule.mk>
