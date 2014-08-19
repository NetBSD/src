# $NetBSD: Makefile,v 1.1.8.1 2014/08/20 00:02:26 tls Exp $
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
