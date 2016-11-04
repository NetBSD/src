#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:20 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	mlx_pci
IOCONF=	mlx_pci.ioconf

SRCS+=	mlx_pci.c

.include <bsd.kmodule.mk>
