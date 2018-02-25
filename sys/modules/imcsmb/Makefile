#	$NetBSD: Makefile,v 1.1 2018/02/25 08:20:29 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev
.PATH:	${S}/dev/pci
.PATH:	${S}/dev/imcsmb

KMOD=	imcsmb
IOCONF= imcsmb.ioconf

SRCS+=	imcsmb.c

.include <bsd.kmodule.mk>
