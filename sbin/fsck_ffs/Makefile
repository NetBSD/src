#	$NetBSD: Makefile,v 1.48 2017/02/08 16:11:40 rin Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_fsck_ffs.
# such stuff should be into Makefile.common.

.include <bsd.own.mk>

MAN=	fsck_ffs.8
SRCS=	ffs_bswap.c ffs_appleufs.c
SUBDIR=	SMM.doc

.include "Makefile.common"

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
