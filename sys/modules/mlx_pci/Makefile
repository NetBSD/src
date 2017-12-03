#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	mlx_pci
IOCONF=	mlx_pci.ioconf

SRCS+=	mlx_pci.c

.include <bsd.kmodule.mk>
