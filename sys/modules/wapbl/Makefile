#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:01 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	wapbl

CPPFLAGS+=      -DWAPBL

#CWARNFLAGS.clang=	-Wno-conversion

SRCS=	vfs_wapbl.c

.include <bsd.kmodule.mk>
