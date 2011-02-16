# $NetBSD: Makefile,v 1.4 2011/02/16 08:01:52 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	acpibat.ioconf
SRCS=	acpi_bat.c

WARNS=	4

.include <bsd.kmodule.mk>
