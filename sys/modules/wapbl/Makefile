#	$NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:43 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	wapbl

CPPFLAGS+=      -DWAPBL

#CWARNFLAGS.clang=	-Wno-conversion

SRCS=	vfs_wapbl.c

WARNS=	3

.include <bsd.kmodule.mk>
