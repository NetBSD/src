# $NetBSD: Makefile,v 1.3 2010/10/25 07:48:04 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	ACPIBAT.ioconf
SRCS=	acpi_bat.c

WARNS=	4

.include <bsd.kmodule.mk>
