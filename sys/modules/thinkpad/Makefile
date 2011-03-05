# $NetBSD: Makefile,v 1.1.4.1 2011/03/05 20:55:48 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	thinkpad
IOCONF=	thinkpad.ioconf
SRCS=	thinkpad_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
