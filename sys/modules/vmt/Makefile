#	$NetBSD: Makefile,v 1.4 2020/10/27 08:57:11 ryo Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86
.PATH:	${S}/dev/vmt

KMOD=	vmt	
IOCONF=	vmt.ioconf
SRCS=	vmt.c \
	vmt_subr.c

.include <bsd.kmodule.mk>
