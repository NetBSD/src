#	$NetBSD: Makefile,v 1.3.22.3 2017/12/03 11:39:19 jdolecek Exp $
#

LIB=	rumpnet_virtif
COMMENT=Network interface which uses hypercalls for I/O

VIRTIF=	virt

.include "${.CURDIR}/Makefile.virtif"

RUMPCOMP_USER_SRCS=	virtif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
