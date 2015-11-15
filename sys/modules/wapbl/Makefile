#	$NetBSD: Makefile,v 1.1 2015/11/15 03:12:14 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	wapbl

CPPFLAGS+=      -DWAPBL

#CWARNFLAGS.clang=	-Wno-conversion

SRCS=	vfs_wapbl.c

.include <bsd.kmodule.mk>
