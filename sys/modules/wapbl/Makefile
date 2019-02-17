#	$NetBSD: Makefile,v 1.2 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	wapbl

CPPFLAGS+=      -DWAPBL

#CWARNFLAGS.clang=	-Wno-conversion

SRCS=	vfs_wapbl.c

WARNS=	3

.include <bsd.kmodule.mk>
