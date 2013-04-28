#	$NetBSD: Makefile,v 1.4 2013/04/28 10:43:45 pooka Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

RUMPCOMP_USER=	# filewatch

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
