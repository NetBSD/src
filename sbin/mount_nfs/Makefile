#	$NetBSD: Makefile,v 1.18 2006/01/21 10:32:23 dsl Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_nfs
SRCS=	mount_nfs.c
MAN=	mount_nfs.8

.ifndef SMALLPROG
SRCS+=	getnfsargs.c
.else
SRCS+=	udp_xfer.c
SRCS+=  rpc.c net.c
SRCS+=	getnfsargs_small.c

.PATH: ${NETBSDSRCDIR}/sys/lib/libsa
CPPFLAGS+= -I${.CURDIR}  -I${NETBSDSRCDIR}/sys/lib/libsa
CPPFLAGS+= -DNO_PMAP_CACHE
.endif

CPPFLAGS+= -DNFS

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.if defined(notdef)			# XXX no kernel kerb NFS support!
.if defined(KERBEROS)
CPPFLAGS+=-DNFSKERB
DPADD+=	${LIBKRB} ${LIBDES} ${LIBUTIL}
LDADD+=	-lkrb -ldes -lutil
.endif
.else
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>
