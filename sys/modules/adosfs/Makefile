#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/adosfs

KMOD=	adosfs
SRCS=	adlookup.c adutil.c advfsops.c advnops.c

WARNS=	3

.include <bsd.kmodule.mk>
