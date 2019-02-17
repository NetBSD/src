#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/union

KMOD=	union
SRCS=	union_vfsops.c union_vnops.c union_subr.c

WARNS=	3

.include <bsd.kmodule.mk>
