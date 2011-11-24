# $NetBSD: Makefile,v 1.1 2011/11/24 15:54:55 ahoka Exp $
#
.include <bsd.own.mk>

PROG=		mount_chfs
SRCS=		mount_chfs.c fattr.c pathadj.c
MAN=		mount_chfs.8

MOUNT=		${NETBSDSRCDIR}/sbin/mount
.PATH:		${MOUNT}

CPPFLAGS+=	-I${MOUNT}
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
WARNS=		4

.include <bsd.prog.mk>
