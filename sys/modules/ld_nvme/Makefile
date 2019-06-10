#	$NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:31 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_nvme
IOCONF=	ld_nvme.ioconf

SRCS+=	ld_nvme.c

WARNS=	3

.include <bsd.kmodule.mk>
