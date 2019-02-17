#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:57 rin Exp $

.include "../Makefile.inc"

KMOD=	scsiverbose

.PATH:	${S}/dev/scsipi
SRCS=	scsipi_verbose.c

WARNS=	3

.include <bsd.kmodule.mk>
