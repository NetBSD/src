# $NetBSD: Makefile,v 1.2.2.1 2011/06/06 09:09:45 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpilid
IOCONF=	acpilid.ioconf
SRCS=	acpi_lid.c

WARNS=	4

.include <bsd.kmodule.mk>
