#	$NetBSD: Makefile,v 1.5.4.1 2014/05/18 17:46:17 rmind Exp $

.include <bsd.own.mk>

INCSDIR=	/usr/include/rump

.if (${MKRUMP} != "no")
INCS=		rump.h rump_namei.h rump_syscalls.h rump_syscalls_compat.h
INCS+=		rumpdefs.h rumperr.h rumpuser.h rumpvnode_if.h

INCS+=		rumpkern_if_pub.h rumpvfs_if_pub.h rumpnet_if_pub.h
.endif

.include <bsd.kinc.mk>
