#	$NetBSD: Makefile,v 1.1.4.2 2007/08/20 22:07:12 ad Exp $
#

PROG=		ffs

LDADD+=		${RUMPFSLD_FFS} ${RUMPFSLD_UFS}
DPADD+=		${RUMPFSDP_FFS} ${RUMPFSDP_UFS}

RUMPFS_WANT+=	ufs

.include <bsd.prog.mk>
