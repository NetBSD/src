#	$NetBSD: Makefile,v 1.40 2017/02/08 16:11:40 rin Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_newfs.
# such stuff should be into Makefile.common.

.include <bsd.own.mk>

SRCS=	ffs_bswap.c ffs_appleufs.c
MAN=	newfs.8 mount_mfs.8

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	mount_mfs.8 mfs.8

.include "Makefile.common"

.include <bsd.prog.mk>
