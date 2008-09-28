#	$NetBSD: Makefile,v 1.2.4.1 2008/09/28 11:17:13 mjf Exp $

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
