#	$NetBSD: Makefile,v 1.42 2023/06/03 08:52:55 lukem Exp $
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

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
