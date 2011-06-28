#	$NetBSD: Makefile,v 1.2 2011/06/28 18:14:12 ahoka Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/flash

KMOD=	flash
IOCONF=	flash.ioconf
SRCS=	flash.c flash_io.c

.include <bsd.kmodule.mk>
