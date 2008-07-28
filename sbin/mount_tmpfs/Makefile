#	$NetBSD: Makefile,v 1.3 2008/07/28 12:42:12 pooka Exp $

.include <bsd.own.mk>

PROG=		mount_tmpfs
SRCS=		mount_tmpfs.c fattr.c
MAN=		mount_tmpfs.8

MOUNT=		${NETBSDSRCDIR}/sbin/mount
.PATH:		${MOUNT}

CPPFLAGS+=	-I${MOUNT}
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
WARNS=		4

.include <bsd.prog.mk>
