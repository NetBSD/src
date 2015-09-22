#	$NetBSD: Makefile,v 1.7.4.1 2015/09/22 12:06:15 skrll Exp $

.include <bsd.own.mk>

INCSDIR=	/usr/include/rump

.if (${MKRUMP} != "no")
INCS=		rump.h rump_namei.h rump_syscalls.h rump_syscalls_compat.h
INCS+=		rumpdefs.h rumperr.h rumperrno2host.h rumpuser.h rumpvnode_if.h

INCS+=		rumpkern_if_pub.h rumpvfs_if_pub.h rumpnet_if_pub.h
.endif

.include <bsd.kinc.mk>
