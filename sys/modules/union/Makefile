#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:18 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/union

KMOD=	union
SRCS=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmodule.mk>
