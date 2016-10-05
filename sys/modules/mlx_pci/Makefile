#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:07 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	mlx_pci
IOCONF=	mlx_pci.ioconf

SRCS+=	mlx_pci.c

.include <bsd.kmodule.mk>
