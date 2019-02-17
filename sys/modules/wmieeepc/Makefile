# $NetBSD: Makefile,v 1.2 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=	wmieeepc
IOCONF=	wmieeepc.ioconf
SRCS=	wmi_eeepc.c

.include <bsd.kmodule.mk>
