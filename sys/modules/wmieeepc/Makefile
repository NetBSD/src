# $NetBSD: Makefile,v 1.1.2.2 2011/02/17 12:00:48 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=	wmieeepc
IOCONF=	wmieeepc.ioconf
SRCS=	wmi_eeepc.c

WARNS=	4

.include <bsd.kmodule.mk>
