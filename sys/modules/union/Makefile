#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:14 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/union

KMOD=	union
SRCS=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmodule.mk>
