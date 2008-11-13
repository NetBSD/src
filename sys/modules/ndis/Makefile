#	$NetBSD: Makefile,v 1.1 2008/11/13 12:11:54 ad Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/ndis

KMOD=	ndis
SRCS=	kern_ndis.c kern_windrv.c nbcompat.c subr_hal.c subr_ndis.c
SRCS+=	subr_ntoskrnl.c subr_pe.c subr_usbd.c

.include <bsd.kmodule.mk>
