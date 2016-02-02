#	$NetBSD: Makefile,v 1.10 2016/02/02 12:22:23 pooka Exp $

.include <bsd.own.mk>

INCSDIR=	/usr/include/rump

.if (${MKRUMP} != "no")
INCS=		rump.h rump_namei.h rump_syscalls.h rump_syscalls_compat.h
INCS+=		rump_syscallshotgun.h
INCS+=		rumpdefs.h rumperr.h rumperrno2host.h rumpuser.h rumpvnode_if.h
INCS+=		rumpfs.h

INCS+=		rumpkern_if_pub.h rumpvfs_if_pub.h rumpnet_if_pub.h
.endif

.include <bsd.kinc.mk>
