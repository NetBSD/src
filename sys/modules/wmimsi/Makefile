# $NetBSD: Makefile,v 1.3 2011/02/16 08:19:57 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmimsi
IOCONF=	wmimsi.ioconf
SRCS=	wmi_msi.c

WARNS=	4

.include <bsd.kmodule.mk>
