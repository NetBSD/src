#	$NetBSD: Makefile,v 1.11 2002/08/19 10:16:57 lukem Exp $
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
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif
.endif

.include <bsd.prog.mk>
