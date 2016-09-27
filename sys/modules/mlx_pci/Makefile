#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	mlx_pci
IOCONF=	mlx_pci.ioconf

SRCS+=	mlx_pci.c

.include <bsd.kmodule.mk>
