# $NetBSD: Makefile,v 1.3.2.1 2011/06/06 09:09:45 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	acpibat.ioconf
SRCS=	acpi_bat.c

WARNS=	4

.include <bsd.kmodule.mk>
