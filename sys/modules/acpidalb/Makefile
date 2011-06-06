#	$NetBSD: Makefile,v 1.1.4.1 2011/06/06 09:09:45 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpidalb
IOCONF=	acpidalb.ioconf
SRCS=	dalb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
