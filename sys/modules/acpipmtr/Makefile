# $NetBSD: Makefile,v 1.2 2011/02/16 08:01:52 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpipmtr
IOCONF=	acpipmtr.ioconf
SRCS=	acpi_pmtr.c

WARNS=	4

.include <bsd.kmodule.mk>
