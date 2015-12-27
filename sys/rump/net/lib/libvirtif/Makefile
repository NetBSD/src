#	$NetBSD: Makefile,v 1.9.6.1 2015/12/27 12:10:18 skrll Exp $
#

LIB=	rumpnet_virtif
COMMENT=Network interface which uses hypercalls for I/O

VIRTIF=	virt

.include "${.CURDIR}/Makefile.virtif"

RUMPCOMP_USER_SRCS=	virtif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
