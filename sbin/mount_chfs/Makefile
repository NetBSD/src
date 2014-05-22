# $NetBSD: Makefile,v 1.1.4.3 2014/05/22 11:37:29 yamt Exp $
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

.include <bsd.prog.mk>
