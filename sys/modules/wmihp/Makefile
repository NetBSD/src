# $NetBSD: Makefile,v 1.3 2011/02/16 08:19:56 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmihp
IOCONF=	wmihp.ioconf
SRCS=	wmi_hp.c

WARNS=	4

.include <bsd.kmodule.mk>
