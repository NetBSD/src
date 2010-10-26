#	$NetBSD: Makefile,v 1.1 2010/10/26 05:36:49 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpqlb
IOCONF=	HPQLB.ioconf
SRCS=	hpqlb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
