#	$NetBSD: Makefile,v 1.24 2011/04/10 16:47:05 joerg Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

USE_FORT?= yes	# network client
PROG=	mount_nfs
SRCS=	mount_nfs.c pathadj.c
MAN=	mount_nfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-I${MOUNT}
.PATH:	${MOUNT}

.ifndef SMALLPROG
SRCS+=	getnfsargs.c
.else
SRCS+=	udp_xfer.c
SRCS+=	rpc.c net.c
SRCS+=	getnfsargs_small.c

.PATH: ${NETBSDSRCDIR}/sys/lib/libsa
CPPFLAGS+= -I${.CURDIR}  -I${NETBSDSRCDIR}/sys/lib/libsa
CPPFLAGS+= -DNO_PMAP_CACHE
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
