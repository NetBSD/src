#	$NetBSD: Makefile,v 1.5.2.2 2017/12/03 11:39:14 jdolecek Exp $

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
