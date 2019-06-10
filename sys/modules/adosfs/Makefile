#	$NetBSD: Makefile,v 1.1.92.1 2019/06/10 22:09:08 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/adosfs

KMOD=	adosfs
SRCS=	adlookup.c adutil.c advfsops.c advnops.c

WARNS=	3

.include <bsd.kmodule.mk>
