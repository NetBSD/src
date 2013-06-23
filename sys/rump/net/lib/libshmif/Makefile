#	$NetBSD: Makefile,v 1.3.18.1 2013/06/23 06:20:29 tls Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

RUMPCOMP_USER=	# filewatch

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
