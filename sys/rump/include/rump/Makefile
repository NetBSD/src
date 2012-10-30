#	$NetBSD: Makefile,v 1.4.8.1 2012/10/30 17:22:53 yamt Exp $

.include <bsd.own.mk>

INCSDIR=	/usr/include/rump

.if (${MKRUMP} != "no")
INCS=		rump.h rump_namei.h rump_syscalls.h rump_syscalls_compat.h
INCS+=		rumpdefs.h rumpuser.h rumpvnode_if.h

INCS+=		rumpkern_if_pub.h rumpvfs_if_pub.h rumpnet_if_pub.h

INCS+=		scsitest.h
.endif

.include <bsd.kinc.mk>
