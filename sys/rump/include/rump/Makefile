#	$NetBSD: Makefile,v 1.9 2016/01/31 23:14:34 pooka Exp $

.include <bsd.own.mk>

INCSDIR=	/usr/include/rump

.if (${MKRUMP} != "no")
INCS=		rump.h rump_namei.h rump_syscalls.h rump_syscalls_compat.h
INCS+=		rump_syscallshotgun.h
INCS+=		rumpdefs.h rumperr.h rumperrno2host.h rumpuser.h rumpvnode_if.h

INCS+=		rumpkern_if_pub.h rumpvfs_if_pub.h rumpnet_if_pub.h
.endif

.include <bsd.kinc.mk>
