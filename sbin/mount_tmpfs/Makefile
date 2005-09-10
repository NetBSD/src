#	$NetBSD: Makefile,v 1.1 2005/09/10 19:20:51 jmmv Exp $

.include <bsd.own.mk>

PROG=		mount_tmpfs
SRCS=		mount_tmpfs.c
MAN=		mount_tmpfs.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
WARNS=		4

.include <bsd.prog.mk>
