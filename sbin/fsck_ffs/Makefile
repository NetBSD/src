#	$NetBSD: Makefile,v 1.51 2023/06/03 08:52:55 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_fsck_ffs.
# such stuff should be into Makefile.common.

.include <bsd.own.mk>

MAN=	fsck_ffs.8
SRCS=	ffs_bswap.c ffs_appleufs.c
SUBDIR.roff=	SMM.doc

.include "Makefile.common"

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
