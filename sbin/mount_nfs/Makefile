#	$NetBSD: Makefile,v 1.12 2002/09/21 18:43:36 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_nfs
SRCS=	mount_nfs.c getmntopts.c
MAN=	mount_nfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -DNFS -I${MOUNT}
.PATH:	${MOUNT}

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
