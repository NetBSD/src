#	$NetBSD: Makefile,v 1.20 2006/03/23 23:53:23 wiz Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_nfs
SRCS=	mount_nfs.c
MAN=	mount_nfs.8

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

CPPFLAGS+= -DNFS

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
