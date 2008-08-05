#	$NetBSD: Makefile,v 1.4 2008/08/05 20:57:45 pooka Exp $

.include <bsd.own.mk>

PROG=		mount_tmpfs
SRCS=		mount_tmpfs.c fattr.c pathadj.c
MAN=		mount_tmpfs.8

MOUNT=		${NETBSDSRCDIR}/sbin/mount
.PATH:		${MOUNT}

CPPFLAGS+=	-I${MOUNT}
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
WARNS=		4

.include <bsd.prog.mk>
