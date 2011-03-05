# $NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:49 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=	wmieeepc
IOCONF=	wmieeepc.ioconf
SRCS=	wmi_eeepc.c

WARNS=	4

.include <bsd.kmodule.mk>
