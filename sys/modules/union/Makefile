#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:36 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/union

KMOD=	union
SRCS=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmodule.mk>
