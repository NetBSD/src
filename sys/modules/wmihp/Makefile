# $NetBSD: Makefile,v 1.2.6.1 2011/02/17 12:00:49 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmihp
IOCONF=	wmihp.ioconf
SRCS=	wmi_hp.c

WARNS=	4

.include <bsd.kmodule.mk>
