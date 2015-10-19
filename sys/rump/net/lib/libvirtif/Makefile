#	$NetBSD: Makefile,v 1.10 2015/10/19 16:16:39 pooka Exp $
#

LIB=	rumpnet_virtif
COMMENT=Network interface which uses hypercalls for I/O

VIRTIF=	virt

.include "${.CURDIR}/Makefile.virtif"

RUMPCOMP_USER_SRCS=	virtif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
