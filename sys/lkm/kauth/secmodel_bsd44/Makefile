# $NetBSD: Makefile,v 1.1 2007/01/17 11:28:09 elad Exp $

.include "../Makefile.inc"

.PATH:	$S/secmodel/bsd44 ${.CURDIR}/..

KMOD=	secmodel_bsd44_lkm

SRCS=	secmodel_bsd44_lkminit.c
SRCS+=	secmodel_bsd44.c secmodel_bsd44_logic.c secmodel_bsd44_suser.c \
	secmodel_bsd44_securelevel.c

.include <bsd.kmod.mk>
