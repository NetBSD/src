# $NetBSD: Makefile,v 1.3.2.2 2011/03/05 20:55:50 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmimsi
IOCONF=	wmimsi.ioconf
SRCS=	wmi_msi.c

WARNS=	4

.include <bsd.kmodule.mk>
