#	$NetBSD: Makefile,v 1.9 1997/09/16 12:31:11 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_nfs
SRCS=	mount_nfs.c getmntopts.c
MAN=	mount_nfs.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -DNFS -I${MOUNT}
.PATH:	${MOUNT}

.if defined(notdef)			# XXX no kernel kerb NFS support!
.if defined(KERBEROS)
CFLAGS+=-DNFSKERB
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif
.endif

.include <bsd.prog.mk>
