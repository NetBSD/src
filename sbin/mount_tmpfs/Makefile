#	$NetBSD: Makefile,v 1.1.12.1 2007/12/27 00:47:01 mjf Exp $

.include <bsd.own.mk>

PROG=		mount_tmpfs
SRCS=		mount_tmpfs.c fattr.c
MAN=		mount_tmpfs.8

MOUNT=		${NETBSDSRCDIR}/sbin/mount
.PATH:		${MOUNT}

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${MOUNT}
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
WARNS=		4

.include <bsd.prog.mk>
