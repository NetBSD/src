#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:59 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/union

KMOD=	union
SRCS=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmodule.mk>
