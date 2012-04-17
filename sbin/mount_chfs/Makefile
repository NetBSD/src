# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:05:41 yamt Exp $
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
