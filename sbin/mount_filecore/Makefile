#	$NetBSD: Makefile,v 1.4 2002/08/19 10:16:57 lukem Exp $
#
#	Makefile	1.0	1998/6/26

.include <bsd.own.mk>

PROG=	mount_filecore
SRCS=	mount_filecore.c getmntopts.c fattr.c
MAN=	mount_filecore.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH: ${MOUNT}

.include <bsd.prog.mk>
