#	$NetBSD: Makefile,v 1.7 1997/05/06 20:45:14 gwr Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_nfs
SRCS=	mount_nfs.c getmntopts.c
MAN=	mount_nfs.8

# Temporarily removed to use own getmntops.c - fvdl
#
#MOUNT=	${.CURDIR}/../mount
#CFLAGS+= -DNFS -I${MOUNT}
#.PATH.c: ${MOUNT}
CFLAGS+=-DNFS

.if defined(notdef)			# XXX no kernel kerb NFS support!
.if defined(KERBEROS)
CFLAGS+=-DNFSKERB
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif
.endif

.include <bsd.prog.mk>
