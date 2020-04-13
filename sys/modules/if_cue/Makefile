#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:12 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_cue
IOCONF=	if_cue.ioconf
SRCS=	if_cue.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
