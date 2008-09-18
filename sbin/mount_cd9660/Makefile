#	$NetBSD: Makefile,v 1.11.24.1 2008/09/18 04:28:26 wrstuden Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_cd9660
SRCS=	mount_cd9660.c pathadj.c
MAN=	mount_cd9660.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
