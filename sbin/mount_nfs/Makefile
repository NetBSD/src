#	from: @(#)Makefile	8.2 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.2 1994/07/27 03:15:31 brezak Exp $

PROG=	mount_nfs
SRCS=	mount_nfs.c getmntopts.c
MAN8=	mount_nfs.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -DNFS -I${MOUNT}
.PATH:	${MOUNT}

.if defined(KERBEROS)
CFLAGS+=-DKERBEROS
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif

.include <bsd.prog.mk>
