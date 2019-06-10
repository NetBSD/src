#	$NetBSD: Makefile,v 1.1.92.1 2019/06/10 22:09:42 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/union

KMOD=	union
SRCS=	union_vfsops.c union_vnops.c union_subr.c

WARNS=	3

.include <bsd.kmodule.mk>
