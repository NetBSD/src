#	$NetBSD: Makefile,v 1.1 2011/10/17 22:39:23 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	vmt	
IOCONF=	vmt.ioconf
SRCS=	vmt.c
COPTS.vmt.c=	-O0

WARNS=	4

.include <bsd.kmodule.mk>
