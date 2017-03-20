#	$NetBSD: Makefile,v 1.46.2.1 2017/03/20 06:57:01 pgoyette Exp $
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
