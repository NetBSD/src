#	$NetBSD$

.include <bsd.own.mk>

PROG=	mount_full
SRCS=	mount_full.c pathadj.c
MAN=	mount_full.8

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:  ${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
