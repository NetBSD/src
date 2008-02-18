#	$NetBSD: Makefile,v 1.1.2.1 2008/02/18 22:07:02 mjf Exp $

.include <bsd.own.mk>

PROG=		mount_devfs
SRCS=		mount_devfs.c fattr.c
MAN=		mount_devfs.8

MOUNT=		${NETBSDSRCDIR}/sbin/mount
.PATH:		${MOUNT}

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${MOUNT}
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
WARNS=		4

.include <bsd.prog.mk>
