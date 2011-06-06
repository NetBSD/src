# $NetBSD: Makefile,v 1.2.2.1 2011/06/06 09:09:50 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	thinkpad
IOCONF=	thinkpad.ioconf
SRCS=	thinkpad_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
