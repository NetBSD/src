# $Id: Makefile,v 1.1.1.1 2003/02/16 08:15:08 jdolecek Exp $

PROG=	mount_smbfs
SRCS=	mount_smbfs.c getmntopts.c
MAN8=	mount_smbfs.8

BINDIR=	/sbin
#NOSHARED=yes

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -DSMBFS -I${MOUNT}

.PATH:	${MOUNT}

LDADD+= -lsmb
DPADD+= ${LIBSMB}

.include <bsd.prog.mk>
